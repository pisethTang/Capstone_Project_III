package main

import (
	"encoding/json"
	"errors"
	"net/http"
	"net/http/httptest"
	"path/filepath"
	"strings"
	"testing"

	"github.com/gin-gonic/gin"
)

type engineCall struct {
	projectRoot string
	enginePath  string
	args        []string
}

func newTestRouter(deps appDeps) *gin.Engine {
	gin.SetMode(gin.TestMode)
	return buildRouter(deps)
}

func performRequest(router http.Handler, method, path, body string) *httptest.ResponseRecorder {
	req := httptest.NewRequest(method, path, strings.NewReader(body))
	req.Header.Set("Content-Type", "application/json")
	w := httptest.NewRecorder()
	router.ServeHTTP(w, req)
	return w
}

func decodeJSONBody(t *testing.T, w *httptest.ResponseRecorder) map[string]any {
	t.Helper()
	out := map[string]any{}
	if err := json.Unmarshal(w.Body.Bytes(), &out); err != nil {
		t.Fatalf("failed to decode JSON response: %v", err)
	}
	return out
}

func TestHealthEndpointReturnsOk(t *testing.T) {
	router := newTestRouter(appDeps{
		resolveProjectRoot: func() (string, error) {
			return "/tmp/project", nil
		},
		runEngine: func(projectRoot, enginePath string, args ...string) ([]byte, error) {
			t.Fatalf("runEngine should not be called for health endpoint")
			return nil, nil
		},
		readFile: func(path string) ([]byte, error) {
			t.Fatalf("readFile should not be called for health endpoint")
			return nil, nil
		},
	})

	w := performRequest(router, http.MethodGet, "/health", "")
	if w.Code != http.StatusOK {
		t.Fatalf("expected status 200, got %d", w.Code)
	}

	body := decodeJSONBody(t, w)
	if got, ok := body["status"].(string); !ok || got != "ok" {
		t.Fatalf("expected status=ok, got %#v", body["status"])
	}
}

func TestInvalidJSONReturnsBadRequestForAllPostRoutes(t *testing.T) {
	routes := []string{"/compute", "/analytics", "/heat"}

	for _, route := range routes {
		t.Run(route, func(t *testing.T) {
			runCalled := false
			router := newTestRouter(appDeps{
				resolveProjectRoot: func() (string, error) {
					return "/tmp/project", nil
				},
				runEngine: func(projectRoot, enginePath string, args ...string) ([]byte, error) {
					runCalled = true
					return nil, nil
				},
				readFile: func(path string) ([]byte, error) {
					return []byte(`{"ok":true}`), nil
				},
			})

			w := performRequest(router, http.MethodPost, route, "{")
			if w.Code != http.StatusBadRequest {
				t.Fatalf("expected status 400, got %d", w.Code)
			}
			if runCalled {
				t.Fatalf("runEngine should not be called for invalid JSON on %s", route)
			}
		})
	}
}

func TestComputeSanitizesModelPathTraversalInput(t *testing.T) {
	const projectRoot = "/tmp/project"
	var got engineCall

	router := newTestRouter(appDeps{
		resolveProjectRoot: func() (string, error) {
			return projectRoot, nil
		},
		runEngine: func(projectRootArg, enginePath string, args ...string) ([]byte, error) {
			got.projectRoot = projectRootArg
			got.enginePath = enginePath
			got.args = append([]string(nil), args...)
			return []byte(""), nil
		},
		readFile: func(path string) ([]byte, error) {
			return []byte(`{"ok":true}`), nil
		},
	})

	body := `{"start":1,"end":2,"model":"../../etc/passwd"}`
	w := performRequest(router, http.MethodPost, "/compute", body)
	if w.Code != http.StatusOK {
		t.Fatalf("expected status 200, got %d", w.Code)
	}

	if got.projectRoot != projectRoot {
		t.Fatalf("expected projectRoot %q, got %q", projectRoot, got.projectRoot)
	}

	expectedEngine := filepath.Join(projectRoot, "main")
	if got.enginePath != expectedEngine {
		t.Fatalf("expected enginePath %q, got %q", expectedEngine, got.enginePath)
	}

	if len(got.args) != 3 {
		t.Fatalf("expected 3 args, got %d (%v)", len(got.args), got.args)
	}

	expectedModelPath := filepath.Join(projectRoot, "frontend", "public", "data", "passwd")
	if got.args[2] != expectedModelPath {
		t.Fatalf("expected sanitized model path %q, got %q", expectedModelPath, got.args[2])
	}
}

func TestRouteModeAndResultMappingForAnalyticsAndHeat(t *testing.T) {
	const projectRoot = "/tmp/project"
	cases := []struct {
		name       string
		route      string
		expected   string
		resultFile string
	}{
		{name: "analytics", route: "/analytics", expected: "analytics", resultFile: "analytics.json"},
		{name: "heat", route: "/heat", expected: "heat", resultFile: "heat_result.json"},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			var got engineCall
			readPath := ""
			router := newTestRouter(appDeps{
				resolveProjectRoot: func() (string, error) {
					return projectRoot, nil
				},
				runEngine: func(projectRootArg, enginePath string, args ...string) ([]byte, error) {
					got.projectRoot = projectRootArg
					got.enginePath = enginePath
					got.args = append([]string(nil), args...)
					return []byte(""), nil
				},
				readFile: func(path string) ([]byte, error) {
					readPath = path
					return []byte(`{"ok":true}`), nil
				},
			})

			w := performRequest(router, http.MethodPost, tc.route,
				`{"start":4,"end":9,"model":"mesh.obj"}`)
			if w.Code != http.StatusOK {
				t.Fatalf("expected status 200, got %d", w.Code)
			}

			expectedModelPath := filepath.Join(projectRoot, "frontend", "public", "data", "mesh.obj")
			expectedArgs := []string{"4", "9", expectedModelPath, tc.expected}
			if len(got.args) != len(expectedArgs) {
				t.Fatalf("expected args %v, got %v", expectedArgs, got.args)
			}
			for i := range expectedArgs {
				if got.args[i] != expectedArgs[i] {
					t.Fatalf("expected args %v, got %v", expectedArgs, got.args)
				}
			}

			expectedReadPath := filepath.Join(projectRoot, "frontend", "public", tc.resultFile)
			if readPath != expectedReadPath {
				t.Fatalf("expected readFile path %q, got %q", expectedReadPath, readPath)
			}
		})
	}
}

func TestComputeEngineFailureReturnsErrorAndModelPath(t *testing.T) {
	const projectRoot = "/tmp/project"
	router := newTestRouter(appDeps{
		resolveProjectRoot: func() (string, error) {
			return projectRoot, nil
		},
		runEngine: func(projectRootArg, enginePath string, args ...string) ([]byte, error) {
			return []byte("engine exploded\n"), errors.New("exit status 1")
		},
		readFile: func(path string) ([]byte, error) {
			t.Fatalf("readFile should not be called when engine execution fails")
			return nil, nil
		},
	})

	w := performRequest(router, http.MethodPost, "/compute",
		`{"start":0,"end":1,"model":"../evil.obj"}`)
	if w.Code != http.StatusInternalServerError {
		t.Fatalf("expected status 500, got %d", w.Code)
	}

	body := decodeJSONBody(t, w)
	if got, ok := body["error"].(string); !ok || got != "engine exploded" {
		t.Fatalf("expected error 'engine exploded', got %#v", body["error"])
	}

	expectedModelPath := filepath.Join(projectRoot, "frontend", "public", "data", "evil.obj")
	if got, ok := body["modelPath"].(string); !ok || got != expectedModelPath {
		t.Fatalf("expected modelPath %q, got %#v", expectedModelPath, body["modelPath"])
	}
}

func TestComputeEngineFailureFallsBackToErrorTextWhenOutputEmpty(t *testing.T) {
	router := newTestRouter(appDeps{
		resolveProjectRoot: func() (string, error) {
			return "/tmp/project", nil
		},
		runEngine: func(projectRootArg, enginePath string, args ...string) ([]byte, error) {
			return []byte("\n\t"), errors.New("exit status 7")
		},
		readFile: func(path string) ([]byte, error) {
			t.Fatalf("readFile should not be called when engine execution fails")
			return nil, nil
		},
	})

	w := performRequest(router, http.MethodPost, "/compute",
		`{"start":0,"end":1,"model":"mesh.obj"}`)
	if w.Code != http.StatusInternalServerError {
		t.Fatalf("expected status 500, got %d", w.Code)
	}

	body := decodeJSONBody(t, w)
	if got, ok := body["error"].(string); !ok || got != "exit status 7" {
		t.Fatalf("expected error 'exit status 7', got %#v", body["error"])
	}
}

func TestResolveProjectRootFailureReturnsInternalServerError(t *testing.T) {
	runCalled := false
	router := newTestRouter(appDeps{
		resolveProjectRoot: func() (string, error) {
			return "", errors.New("cannot resolve")
		},
		runEngine: func(projectRootArg, enginePath string, args ...string) ([]byte, error) {
			runCalled = true
			return nil, nil
		},
		readFile: func(path string) ([]byte, error) {
			return nil, nil
		},
	})

	w := performRequest(router, http.MethodPost, "/compute",
		`{"start":0,"end":1,"model":"mesh.obj"}`)
	if w.Code != http.StatusInternalServerError {
		t.Fatalf("expected status 500, got %d", w.Code)
	}
	if runCalled {
		t.Fatalf("runEngine should not be called when project root resolution fails")
	}

	body := decodeJSONBody(t, w)
	if got, ok := body["error"].(string); !ok || got != "failed to determine project root" {
		t.Fatalf("expected project-root error message, got %#v", body["error"])
	}
}

func TestReadFileFailureReturnsRouteSpecificMessage(t *testing.T) {
	routes := []struct {
		route       string
		expectedErr string
	}{
		{route: "/compute", expectedErr: "failed to read result.json"},
		{route: "/analytics", expectedErr: "failed to read analytics.json"},
		{route: "/heat", expectedErr: "failed to read heat_result.json"},
	}

	for _, tc := range routes {
		t.Run(tc.route, func(t *testing.T) {
			router := newTestRouter(appDeps{
				resolveProjectRoot: func() (string, error) {
					return "/tmp/project", nil
				},
				runEngine: func(projectRootArg, enginePath string, args ...string) ([]byte, error) {
					return []byte(""), nil
				},
				readFile: func(path string) ([]byte, error) {
					return nil, errors.New("missing file")
				},
			})

			w := performRequest(router, http.MethodPost, tc.route,
				`{"start":0,"end":1,"model":"mesh.obj"}`)
			if w.Code != http.StatusInternalServerError {
				t.Fatalf("expected status 500, got %d", w.Code)
			}

			body := decodeJSONBody(t, w)
			if got, ok := body["error"].(string); !ok || got != tc.expectedErr {
				t.Fatalf("expected error %q, got %#v", tc.expectedErr, body["error"])
			}
		})
	}
}

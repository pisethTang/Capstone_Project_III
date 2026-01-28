package main

import (
	"os"
	"path/filepath"
)

func resolveProjectRoot() (string, error) {
	if exe, err := os.Executable(); err == nil {
		exeDir := filepath.Dir(exe)
		return filepath.Clean(filepath.Join(exeDir, "..")), nil
	}
	wd, err := os.Getwd()
	if err != nil {
		return "", err
	}
	return filepath.Clean(filepath.Join(wd, "..")), nil
}

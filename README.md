# Capstone_Project_III
This repository stores the following, but not limited to. for my Math's  Capstone Course III: course notes, scripts to animate (and explain) various math objects in C, C++ and Python. 

## LaTeX Development Setup with VS Code

Follow these steps to set up a robust LaTeX writing environment in this project.

### 1. Install LaTeX Distribution (Linux/WSL)
Since this project runs in a Linux/WSL environment, you must install the LaTeX compiler and build tools natively in the terminal.

Run the following command to install `texlive` and `latexmk`:

```bash
sudo apt update
sudo apt install texlive-latex-extra texlive-fonts-recommended texlive-fonts-extra latexmk
```

### 2. VS Code Extension

**Required Extension:**
- **LaTeX Workshop** (`james-yu.latex-workshop`): The all-in-one tool for building, viewing, and linting LaTeX.

### 3. Usage Guide

- **Build Project:** 
  - Command: `Ctrl + Alt + B`
  - This compiles the `.tex` file using `latexmk`.

- **View PDF:** 
  - Command: `Ctrl + Alt + V` (Select "View in VS Code tab")
  - This opens the PDF in a split pane to the right.

- **Reverse Sync (Jump to Code):**
  - Hold `Ctrl` (or `Cmd` on Mac) and **Click** on any text in the PDF viewer.
  - The cursor in the text editor will jump to the corresponding line in the source code.

### 4. Configuration: Auto-Build on Save
To make the PDF update automatically whenever you save the file:
1. Open Settings (`Ctrl + ,`).
2. Search for `latex auto build`.
3. Set **Latex-workshop > Latex > Auto Build: Run** to `onSave`.

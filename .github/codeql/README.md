# CodeQL Setup for deepiri-egottol

This folder contains the CodeQL configuration for security scanning in this repository.

## What each file does

- `.github/workflows/codeql.yml`
  - Defines when scans run and how GitHub Actions executes CodeQL for C/C++ code.
- `.github/codeql/codeql-config.yml`
  - Defines what folders to include and ignore during analysis.

## CodeQL workflow breakdown (`.github/workflows/codeql.yml`)

### `name: CodeQL`
The display name in the Actions tab.

### `on.pull_request.branches` and `on.push.branches`
```yaml
on:
  pull_request:
    branches: [main, dev]
  push:
    branches: [main, dev]
```
Runs scans when PRs target `main` or `dev`, and when commits are pushed to `main` or `dev`.

### `permissions`
```yaml
permissions:
  actions: read
  contents: read
  security-events: write
```
Uses least-privilege permissions. `security-events: write` is required so CodeQL can upload findings.

### Language setup (current)
```yaml
with:
  languages: cpp,c
```
This workflow currently runs analysis for C and C++.

### Checkout step
```yaml
with:
  fetch-depth: 0
```
- `fetch-depth: 0` keeps full git history (safe default for analysis and troubleshooting).

### Initialize CodeQL
```yaml
uses: github/codeql-action/init@v3
with:
  config-file: ./.github/codeql/codeql-config.yml
```
Starts the CodeQL engine and loads `.github/codeql/codeql-config.yml`.

### Analyze
```yaml
uses: github/codeql-action/analyze@v3
```
Executes queries and uploads results to GitHub Security.

## Config breakdown (`.github/codeql/codeql-config.yml`)

### `paths-ignore`
Generated/build/runtime artifact paths are excluded to reduce noise and runtime:

```yaml
paths-ignore:
  - '**/node_modules/**'
  - '**/dist/**'
  - '**/build/**'
  - '**/coverage/**'
  - '**/logs/**'
  - '**/*.min.js'
  - '**/*.min.css'
  - '**/*.o'
  - '**/*.obj'
  - '**/*.exe'
  - '**/*.dll'
  - '**/*.so'
  - '**/CMakeFiles/**'
  - '**/cmake-build-*/**'
  - '**/.vscode/**'
  - '**/.idea/**'
  - '**/.git/**'
  - '**/__pycache__/**'
  - '**/*.pyc'
  - '**/*.pyo'
  - '**/*.egg-info/**'
  - '**/.mypy_cache/**'
  - '**/.pytest_cache/**'
```

## Best practices

1. **Keep trigger scope intentional.**
   Use branch filters (`main`, `dev`) to control cost and noise.
2. **Keep language list explicit.**
   Only include languages with meaningful source code.
3. **Exclude generated/vendor artifacts.**
   Keep caches, dependencies, build outputs, logs, and minified files in `paths-ignore`.
4. **Pin to stable major action versions.**
   `@v3` is the current stable major for CodeQL actions.
5. **Review alerts regularly.**
   Triage high/critical findings first and suppress only with documented reasoning.

## Maintenance examples
Keeping this updated as code and language coverage evolve is important. Here are common maintenance changes.

### Keep language scope aligned with this repository
This workflow currently analyzes C and C++ only:

```yaml
with:
  languages: cpp,c
```

Only change this value when this repository adds production code in another supported language.

### Exclude another generated folder
Add a glob to `paths-ignore`, for example:

```yaml
- '**/generated/**'
```

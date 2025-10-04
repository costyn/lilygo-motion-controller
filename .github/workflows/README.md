# GitHub Actions Workflows

This directory contains automated CI/CD workflows for the LilyGo Motion Controller project.

## 📋 Workflows

### 🧪 test.yml - Test Suite
**Triggers:** Push to main/dev/adding-unit-tests branches, Pull Requests to main/dev

**Purpose:** Runs automated tests on every code change to ensure quality.

**Jobs:**
1. **cpp-tests** - C++ Native Unit Tests
   - Runs PlatformIO native tests (25 tests)
   - Tests: Configuration module, MotorController calculations
   - Environment: Ubuntu Latest with Python 3.11
   - Uses PlatformIO cache for faster builds

2. **webapp-tests** - WebApp Tests with Coverage
   - Runs Vitest test suite (82 tests)
   - Tests: React components, hooks, WebSocket protocol compliance
   - Generates coverage report (current: ~61%)
   - Environment: Ubuntu Latest with Node.js 20
   - Uses npm cache for faster dependency installation

3. **test-summary** - Quality Gate
   - Aggregates results from all test jobs
   - Fails if any test job fails
   - Generates summary in GitHub Actions output

**Usage:**
```bash
# Tests run automatically on:
git push origin main           # Triggers workflow
git push origin dev            # Triggers workflow
# Opening PR to main/dev       # Triggers workflow
```

### 🏗️ build.yml - Build & Release
**Triggers:** Push tags matching `v*` (e.g., v1.0.0)

**Purpose:** Builds firmware and creates GitHub releases for version tags.

**Jobs:**
1. **build** - Compile Firmware
   - Builds firmware for pico32 environment
   - Builds SPIFFS filesystem (web interface)
   - Uploads artifacts (firmware.bin, spiffs.bin)

2. **release** - Create GitHub Release
   - Creates release with version tag
   - Attaches firmware binaries
   - Generates SHA256 checksums
   - Adds installation instructions

**Usage:**
```bash
# Create release:
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
```

---

## 📊 Test Coverage

### Current Coverage (as of 2025-10-04)
- **Total Tests:** 107 passing
  - C++ Tests: 25
  - WebApp Tests: 82

### WebApp Coverage Breakdown
```
Overall: 61.32%
- Components: 68-92% (tested components)
- Hooks: 85%
- UI Components: 85%
- Utils: 29% (room for improvement)
```

### Coverage Reports
- Local: Run `npm test -- --coverage` in webapp directory
- CI/CD: Coverage uploaded to Codecov (if token configured)

---

## 🔧 Local Testing

### Run All Tests Locally

**C++ Tests:**
```bash
# From project root
pio test -e native --verbose
```

**WebApp Tests:**
```bash
# From webapp directory
cd webapp
npm test                    # Watch mode
npm test -- --run          # Run once
npm test -- --coverage     # With coverage
```

### Debug Failed CI Tests
1. Check GitHub Actions tab in repository
2. Click on failed workflow run
3. Expand failed job to see error details
4. Reproduce locally using commands above

---

## 🚀 Adding New Tests

### C++ Tests
1. Create test file in `test/test_native/test_<module>/`
2. Follow Unity testing framework conventions
3. Add test environment to `platformio.ini` if needed
4. Tests run automatically in CI on next push

### WebApp Tests
1. Create test file alongside component: `Component.test.tsx`
2. Follow Vitest + Testing Library patterns
3. Use existing mocks in `webapp/src/test/setup.ts`
4. Tests run automatically in CI on next push

---

## 🛠️ Workflow Maintenance

### Updating Dependencies
- **PlatformIO:** Cache key uses `platformio.ini` hash
- **npm:** Cache key uses `package-lock.json` hash
- Clear cache by changing `platformio.ini` or `package-lock.json`

### Modifying Triggers
Edit `on:` section in workflow YAML:
```yaml
on:
  push:
    branches: [ main, dev ]  # Add/remove branches
  pull_request:
    branches: [ main ]
```

### Adding New Test Environments
1. Add environment to `platformio.ini`
2. Add test job to `test.yml`
3. Update test-summary to include new job

---

## 📈 Quality Gates

### Test Requirements
- ✅ All C++ tests must pass (25/25)
- ✅ All WebApp tests must pass (82/82)
- ✅ No test warnings (e.g., React `act()` warnings)

### Merge Requirements
- All tests passing in CI
- No breaking changes to existing tests
- New features include tests

### Coverage Thresholds (Future)
- Target: >70% webapp coverage
- Target: >80% C++ module coverage
- Critical paths: 100% coverage

---

## 🐛 Troubleshooting

### Common Issues

**1. PlatformIO Cache Issues**
```bash
# Clear local cache
pio system prune
```

**2. npm Cache Issues**
```bash
# Clear npm cache
cd webapp
rm -rf node_modules package-lock.json
npm install
```

**3. Test Failures Only in CI**
- Check Node/Python version matches CI
- Ensure all dependencies in package.json
- Review test logs for environment-specific issues

**4. Timeout Errors**
- C++ tests: Increase timeout in platformio.ini
- WebApp tests: Add `{ timeout: 10000 }` to waitFor()

---

## 📚 References

- [PlatformIO Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/)
- [Vitest Documentation](https://vitest.dev/)
- [React Testing Library](https://testing-library.com/react)
- [GitHub Actions](https://docs.github.com/en/actions)
- [Test Coverage Plan](../../requirements/2025-10-04-0824-tech-debt-review/08-unit-test-coverage-plan.md)

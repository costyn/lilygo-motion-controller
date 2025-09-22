# Discovery Questions

## Q1: Should the debug WebSocket stream include all existing serial output, or only specific diagnostic messages?
**Default if unknown:** All existing serial output (comprehensive debugging information for development)

## Q2: Will developers primarily use the debug WebSocket stream from a web browser, or from specialized debugging tools?
**Default if unknown:** Web browser (matches existing WebSocket architecture and accessibility)

## Q3: Should mDNS support work alongside the existing WiFiManager configuration portal?
**Default if unknown:** Yes (complementary feature that enhances existing WiFi connectivity)

## Q4: Will unit tests need to run on actual ESP32 hardware, or can they be cross-platform tests for calculation functions?
**Default if unknown:** Cross-platform tests (focus on calculation functions as specified, hardware testing is separate concern)

## Q5: Should the unified timestamp format be configurable, or use a standard format across all modules?
**Default if unknown:** Standard format (consistency is more valuable than configurability for debugging)
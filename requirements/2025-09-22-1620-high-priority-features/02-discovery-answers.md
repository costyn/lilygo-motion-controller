# Discovery Answers

## Q1: Should the debug WebSocket stream include all existing serial output, or only specific diagnostic messages?
**Answer:** All existing serial output (comprehensive debugging information for development)

## Q2: Will developers primarily use the debug WebSocket stream from a web browser, or from specialized debugging tools?
**Answer:** Web browser for now

## Q3: Should mDNS support work alongside the existing WiFiManager configuration portal?
**Answer:** Yes - User provided helpful mDNS implementation snippet:
```cpp
void start_mdns_service()
{
    // initialize mDNS service
    esp_err_t err = mdns_init();
    if (err)
    {
        printf("MDNS Init failed: %d\n", err);
        return;
    }

    // set hostname
    mdns_hostname_set("Lumifera");
    // set default instance
    mdns_instance_name_set("Lumifera");
    mdns_service_add("Lumifera", "_http", "_tcp", 80, NULL, 0);
    // mdns_service_add("Lumifera", "_wss", "_tcp", 80, NULL, 0);
}
```

## Q4: Will unit tests need to run on actual ESP32 hardware, or can they be cross-platform tests for calculation functions?
**Answer:** Cross-platform tests on dev machine (ESP32 memory/flash constraints make on-device testing impractical)

## Q5: Should the unified timestamp format be configurable, or use a standard format across all modules?
**Answer:** Standard format as exemplified in TODO.md
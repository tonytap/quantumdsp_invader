#pragma once

#include <LicenseSpring/Configuration.h>

struct AppConfig
{
    AppConfig() {}
    AppConfig( const std::string& name, const std::string& version )
        : appName( name ), appVersion( version ) {}

    // Create LicenseSpring configuration
    LicenseSpring::Configuration::ptr_t createLicenseSpringConfig() const;

    // LicenseSpring policy codes
    // TODO: Replace this with your actual trial policy code from LicenseSpring dashboard
    static constexpr const char* TRIAL_POLICY_CODE = "trial";

    std::string appName;
    std::string appVersion;
};

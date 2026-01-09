#include "AppConfig.h"
#include "defines.h"
#include <LicenseSpring/EncryptString.h>

LicenseSpring::Configuration::ptr_t AppConfig::createLicenseSpringConfig() const
{
    // Optionally you can provide full path where license file will be stored, hardwareID and other options
    LicenseSpring::ExtendedOptions options;
    options.collectNetworkInfo( true );
    // options.enableLogging( true );
    // options.enableVMDetection( true );

    // In order to connect to the LS FloatingServer set its address as following
    // options.setAlternateServiceURL( "http://localhost:8080" );
    // If you use FloatingServer v1.1.9 or earlier provide Product name instead of Product code

    // Provide your LicenseSpring credentials here, please keep them safe
    return LicenseSpring::Configuration::Create(
        EncryptStr( LS_API_KEY ), // your LicenseSpring API key (UUID)
        EncryptStr( LS_SHARED_KEY ), // your LicenseSpring Shared key
        EncryptStr( LS_PRODUCT_CODE ), // product code that you specified in LicenseSpring for your application
        appName, appVersion, options );
}

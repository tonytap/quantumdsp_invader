#include "licenseChecker.h"
#include "AppConfig.h"
#include <LicenseSpring/Exceptions.h>
using namespace juce;

//==============================================================================
LicenseChecker::LicenseChecker( LicenseSpring::LicenseManager::ptr_t manager, EqAudioProcessor& p) : licenseManager( manager ), audioProcessor(p)
{
    addChildComponent( labelKey );
    labelKey.setJustificationType(juce::Justification::centred);
    addChildComponent( getTrialButton );
    addChildComponent( keyEditor );
    addChildComponent( activateKeyButton );
    addChildComponent(backButton);
    addChildComponent(proceedButton);

    addChildComponent( labelInfo );
    labelInfo.setJustificationType(juce::Justification::centred);
    addChildComponent( deactivateButton );

    getTrialButton.addListener( this );
    deactivateButton.addListener( this );
    activateKeyButton.addListener( this );
    backButton.addListener(this);
    proceedButton.addListener(this);

    getTrialButton.setComponentID( "Get trial" );
    deactivateButton.setComponentID( "Deactivate" );
    activateKeyButton.setComponentID( "Activate key-based" );
    backButton.setComponentID(" Go back ");
    proceedButton.setComponentID(" Proceed ");

    auto license = licenseManager->getCurrentLicense();
    if (license != nullptr) {
        try {
            // Perform an online license check to update the local license state
            license->check();
            if( license != nullptr && license->isValid())
            {
                updateLicenseFields();
                makeInfoVisible();
                audioProcessor.licenseActivated.store(true);
            }
            else {
                makeActivationVisible();
            }
        } catch (const std::exception& ex) {
            DBG(ex.what());
            juce::String message = String(ex.what());
            if (message == "The license is expired.") {
                labelInfo.setText("Trial has expired, please visit QuantumDSP.com to purchase an activation code.", juce::dontSendNotification);
            }
            else {
                labelInfo.setText("Invalid license", juce::dontSendNotification);
            }
            juce::AttributedString attributedText;
            labelInfo.setAttributedText(attributedText);
            makeExpirationInfoVisible();
            audioProcessor.licenseActivated.store(false);
        }
    }
    else {
        makeActivationVisible();
    }

    juce::Colour bgColour((uint8)0x28, (uint8)0x28, (uint8)0x28, (float)1.0f);
    keyEditor.setColour(juce::TextEditor::backgroundColourId, bgColour);
    keyEditor.setJustification(juce::Justification::centred);
    setSize( 800, 600 );
}

LicenseChecker::~LicenseChecker()
{
}

void LicenseChecker::makeInfoVisible()
{
    labelKey.setVisible( false );
    getTrialButton.setVisible( false );
    keyEditor.setVisible( false );
    activateKeyButton.setVisible( false );
    labelInfo.setVisible( true );
    deactivateButton.setVisible( true );
    backButton.setVisible(false);
    proceedButton.setVisible(true);
}

void LicenseChecker::makeExpirationInfoVisible()
{
    labelKey.setVisible( false );
    getTrialButton.setVisible( false );
    keyEditor.setVisible( false );
    activateKeyButton.setVisible( false );
    labelInfo.setVisible( true );
    deactivateButton.setVisible( false );
    backButton.setVisible(true);
    proceedButton.setVisible(false);
}

void LicenseChecker::makeActivationVisible()
{
    labelInfo.setVisible( false );
    deactivateButton.setVisible( false );
    activateKeyButton.setVisible( true );
    labelKey.setVisible( true );
    getTrialButton.setVisible( true );
    keyEditor.setVisible( true );
    backButton.setVisible(false);
    proceedButton.setVisible(false);
}

void LicenseChecker::buttonClicked( juce::Button* button )
{
    auto buttonId = button->getComponentID();
    if( buttonId == activateKeyButton.getComponentID() )
        activateKeyBased();
    if( buttonId == getTrialButton.getComponentID() )
        getTrial();
    if( buttonId == deactivateButton.getComponentID() )
        deactivate();
    if( buttonId == backButton.getComponentID() )
        makeActivationVisible();
    if( buttonId == proceedButton.getComponentID() )
        return;
}

void LicenseChecker::activateKeyBased()
{
    try
    {
        auto key = keyEditor.getText();
        if( !key.isEmpty() )
        {
            LicenseSpring::LicenseID id = LicenseSpring::LicenseID::fromKey( key.toStdString() ) ;
            auto license = licenseManager->activateLicense(id);
            if( license->isValid() )
            {
                updateLicenseFields();
                makeInfoVisible();
                audioProcessor.licenseActivated.store(true);
            }
        }
    }
    catch (const std::exception& ex) {
        juce::String message = String(ex.what());
        if (message == "License validity period has expired.") {
            labelInfo.setText("Trial has expired, please visit QuantumDSP.com to purchase an activation code.", juce::dontSendNotification);
        }
        else {
            labelInfo.setText("Invalid license", juce::dontSendNotification);
        }
        juce::AttributedString attributedText;
        labelInfo.setAttributedText(attributedText);
        makeExpirationInfoVisible();
    }
    catch (...) {
        DBG("Caught unknown exception");
    }
}

void LicenseChecker::getTrial()
{
    try
    {
        // Use the trial policy code to ensure the SDK reuses the cached trial license
        // instead of creating a new trial license on each activation
        auto id = licenseManager->getTrialLicense(AppConfig::TRIAL_POLICY_CODE);
        keyEditor.setText( id.key().c_str() );
    }
    catch( LicenseSpring::LicenseSpringException ex )
    {
        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "LicenseSpring exception encountered: " ) + String( ex.what() ), "Ok" );
    }
    catch( std::exception ex )
    {
        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "Standard exception encountered: " ) + String( ex.what() ), "Ok" );
    }
    catch( ... )
    {
        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "Unknown exception encountered!" ), "Ok" );
    }
}

void LicenseChecker::updateLicenseFields()
{
    try
    {
        auto license = licenseManager->getCurrentLicense();
        if( license == nullptr )
            return;

        labelInfo.setText("", juce::dontSendNotification);
        juce::AttributedString attributedText;
        float titleSize = 20.f;
        float contentSize = 16.f;
        if (license->isTrial()) {
            attributedText.append("Trial Activated\n", juce::Font("Arial", titleSize, juce::Font::bold), juce::Colours::green);
            attributedText.append("\n", juce::Colours::transparentBlack);
            attributedText.append("Days remaining: " + juce::String(license->daysRemaining()), juce::Font("Arial", contentSize, juce::Font::plain), juce::Colours::white);
        }
        else {
            attributedText.append("License Activated\n", juce::Font("Arial", titleSize, juce::Font::bold), juce::Colours::green);
            attributedText.append("\n", juce::Colours::transparentBlack);
            attributedText.append("Click below to deactivate your license for this system. "
                                  "You can reactivate it at any time using the license key provided at purchase.", juce::Font("Arial", contentSize, juce::Font::plain), juce::Colours::white);
        }
        attributedText.setJustification(juce::Justification::centred);
        labelInfo.setAttributedText(attributedText);
    }
    catch( LicenseSpring::LicenseSpringException ex )
    {
        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "LicenseSpring exception encountered: " ) + String( ex.what() ), "Ok" );
    }
    catch( std::exception ex )
    {
        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "Standard exception encountered: " ) + String( ex.what() ), "Ok" );
    }
    catch( ... )
    {
        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "Unknown exception encountered!" ), "Ok" );
    }
}

void LicenseChecker::updateLicenseFieldsWithResize(float sizeFactor) {
    try
    {
        auto license = licenseManager->getCurrentLicense();
        if( license == nullptr )
            return;
        labelInfo.setText("", juce::dontSendNotification);
        juce::AttributedString attributedText;
        float titleSize = sizeFactor*20.f;
        float contentSize = sizeFactor*16.f;
        if (license->isTrial()) {
            attributedText.append("Trial Activated\n", juce::Font("Arial", titleSize, juce::Font::bold), juce::Colours::green);
            attributedText.append("\n", juce::Colours::transparentBlack);
            attributedText.append("Days remaining: " + juce::String(license->daysRemaining()), juce::Font("Arial", contentSize, juce::Font::plain), juce::Colours::white);
        }
        else {
            attributedText.append("License Activated\n", juce::Font("Arial", titleSize, juce::Font::bold), juce::Colours::green);
            attributedText.append("\n", juce::Colours::transparentBlack);
            attributedText.append("Click below to deactivate your license for this system. "
                                  "You can reactivate it at any time using the license key provided at purchase.", juce::Font("Arial", contentSize, juce::Font::plain), juce::Colours::white);
        }
        attributedText.setJustification(juce::Justification::centred);
        labelInfo.setAttributedText(attributedText);
    }
    catch( LicenseSpring::LicenseSpringException ex )
    {
        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "LicenseSpring exception encountered: " ) + String( ex.what() ), "Ok" );
    }
    catch( std::exception ex )
    {
        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "Standard exception encountered: " ) + String( ex.what() ), "Ok" );
    }
    catch( ... )
    {
        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "Unknown exception encountered!" ), "Ok" );
    }
}

void LicenseChecker::deactivate()
{
    try
    {
        licenseManager->getCurrentLicense()->deactivate( true );
    }
    catch( LicenseSpring::LicenseSpringException ex )
    {
        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "LicenseSpring exception encountered: " ) + String( ex.what() ), "Ok" );
    }
    catch( std::exception ex )
    {
        juce::String message = String(ex.what());
        DBG(message);
//        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "Standard exception encountered: " ) + String( ex.what() ), "Ok" );
    }
    catch( ... )
    {
        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "Unknown exception encountered!" ), "Ok" );
    }
    makeActivationVisible();
    audioProcessor.licenseActivated.store(false);
}

void LicenseChecker::checkLicense()
{
    try
    {
        licenseManager->getCurrentLicense()->check();
        updateLicenseFields();
        AlertWindow::showMessageBoxAsync( AlertWindow::InfoIcon, "Info", "License check sucessful!", "Ok" );
    }
    catch( LicenseSpring::LicenseSpringException ex )
    {
        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "LicenseSpring exception encountered: " ) + String( ex.what() ), "Ok" );
    }
    catch( std::exception ex )
    {

        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "Standard exception encountered: " ) + String( ex.what() ), "Ok" );
    }
    catch( ... )
    {
        AlertWindow::showMessageBoxAsync( AlertWindow::WarningIcon, "Error", String( "Unknown exception encountered!" ), "Ok" );
    }
}

//==============================================================================
void LicenseChecker::paint( juce::Graphics& g )
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    juce::Colour bgColour((uint8)0x28, (uint8)0x28, (uint8)0x28, (float)1.0f);
    g.fillAll( bgColour );
    labelKey.setFont(juce::Font("Arial", 16.f*sizePortion, juce::Font::plain));
    keyEditor.setFont(juce::Font("Arial", 16.f*sizePortion, juce::Font::plain));
}

void LicenseChecker::resized()
{
    int column1X = 20;
    int width = ( getWidth() - 80 ) / 3;
    int y = 40;
    y += 110 + 40;
    labelKey.setBoundsRelative( 0.2, 0.05, 0.6, 0.1 );
    keyEditor.setBoundsRelative( 0.2, 0.2, 0.6, 0.1 );
    activateKeyButton.setBoundsRelative( 0.3, 0.35, 0.4, 0.1 );
    getTrialButton.setBoundsRelative( 0.3, 0.5, 0.4, 0.1 );
    labelInfo.setBoundsRelative( 0.05, 0.05, 0.9, 0.8 );
    deactivateButton.setBoundsRelative( 0.4, 0.85, 0.2, 0.1 );
    backButton.setBoundsRelative( 0.4, 0.85, 0.2, 0.1 );
    repaint();
}

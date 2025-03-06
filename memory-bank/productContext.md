# RFID Reader Product Context

## Purpose
The RFID Reader serves as a battery-powered access control system that can identify and authenticate RFID cards/tags, providing both visual and audio feedback while transmitting the results wirelessly to a gateway system.

## Problem Space
1. Need for reliable and power-efficient RFID access control
2. Requirement for immediate user feedback on card scanning
3. Wireless transmission of authentication results
4. Battery-powered operation requiring power optimization

## User Experience Goals
1. **Immediate Feedback**
   - Clear visual indicators through dual LED system
   - Distinct audio feedback through buzzer
   - Different patterns for authorized vs unauthorized cards

2. **Reliability**
   - Consistent card reading using interrupt-based detection
   - Robust error handling for failed transmissions
   - Automatic recovery from communication issues

3. **Power Efficiency**
   - Sleep mode when idle
   - Optimized wake-up on card detection
   - Battery voltage monitoring

## Integration Points
1. **RC522 RFID Module**
   - Handles card detection and reading
   - Uses IRQ pin for efficient card detection
   - Manages card data processing

2. **NRF24L01 Radio**
   - Transmits card data to gateway
   - Handles acknowledgments
   - Manages retry logic for failed transmissions

3. **User Interface**
   - Green LED: Indicates authorized access
   - Red LED: Indicates unauthorized access or errors
   - Buzzer: Provides audio confirmation of scan results

## Expected Behavior
1. **Card Detection**
   - System wakes on card presence
   - Reads card data via RC522
   - Processes card ID for authorization

2. **Authorization**
   - Checks card ID against authorized list
   - Provides immediate feedback:
     * Authorized: Green LED + 2kHz tone (1 second)
     * Unauthorized: Red LED + 400Hz tone (3 blinks/beeps)

3. **Data Transmission**
   - Sends card data to gateway
   - Handles transmission errors
   - Provides error feedback if needed

## Success Indicators
1. User receives clear feedback within 100ms of card scan
2. System reliably detects and reads cards
3. Battery life meets operational requirements
4. Gateway successfully receives card data
5. System recovers automatically from errors

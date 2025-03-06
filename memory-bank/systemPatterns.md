# RFID Reader System Patterns

## System Architecture

```mermaid
graph TD
    subgraph Hardware
        RC[RC522 RFID] --> INT[IRQ Interrupt]
        INT --> MC[Arduino Controller]
        MC --> RF[NRF24L01 Radio]
        MC --> UI[User Interface]
        UI --> LED[LED Indicators]
        UI --> BZ[Buzzer]
    end

    subgraph Power Management
        PS[Power States] --> Sleep[Sleep Mode]
        PS --> Active[Active Mode]
        INT --> Active
    end

    subgraph Data Flow
        CD[Card Detection] --> Auth[Authorization]
        Auth --> FB[Feedback]
        Auth --> TX[Transmission]
    end
```

## Key Design Patterns

### 1. Interrupt-Driven Architecture
- Uses RC522's IRQ pin for efficient card detection
- Replaces polling with interrupt-based wakeup
- Reduces power consumption in idle state

### 2. State Management
```mermaid
stateDiagram-v2
    [*] --> Sleep
    Sleep --> CardDetected: IRQ Interrupt
    CardDetected --> Processing: Read Card
    Processing --> Authorized: Valid Card
    Processing --> Unauthorized: Invalid Card
    Authorized --> Transmitting
    Unauthorized --> Transmitting
    Transmitting --> Sleep
```

### 3. SPI Bus Sharing
- Managed access between RC522 and NRF24L01
- Explicit configuration switching
- Protected transactions using beginTransaction/endTransaction

### 4. Power Management Strategy
- Sleep mode during idle periods
- Wake on interrupt only
- Optimized power states for different operations

### 5. Error Recovery Patterns
```mermaid
graph TD
    Error[Error Detected] --> Check[Check Type]
    Check --> RC[RC522 Error]
    Check --> RF[RF24 Error]
    RC --> RInit[Reinitialize RC522]
    RF --> RTry[Retry Transmission]
    RTry --> RInit[Reinitialize RF24]
    RInit --> Resume[Resume Operation]
```

## Component Interactions

### 1. RC522 RFID Reader
- Primary Patterns:
  * Interrupt-based detection
  * Error recovery with reinitialization
  * SPI bus management

### 2. NRF24L01 Radio
- Primary Patterns:
  * Retry logic for failed transmissions
  * Power state management
  * SPI bus sharing

### 3. User Interface
- Primary Patterns:
  * State-based feedback
  * Synchronized visual and audio signals
  * Error indication

## Code Organization

### 1. Main Loop Structure
```mermaid
graph TD
    Start[Start] --> Check[Check RC522]
    Check --> IRQ{IRQ Triggered?}
    IRQ --> |Yes| Read[Read Card]
    IRQ --> |No| Sleep[Sleep Mode]
    Read --> Process[Process Card]
    Process --> Feedback[User Feedback]
    Process --> Transmit[Transmit Data]
    Feedback --> Sleep
    Transmit --> Sleep
```

### 2. Function Categories
1. **Setup Functions**
   - Hardware initialization
   - Component configuration
   - Interrupt setup

2. **Core Operations**
   - Card detection
   - Authorization
   - Data transmission

3. **Support Functions**
   - SPI configuration
   - Error handling
   - Power management

4. **Feedback Functions**
   - LED control
   - Buzzer patterns
   - Status indication

## Data Structures

### 1. Card Data
```cpp
struct {
  uint32_t card_id;
  bool authorized;
  float batt;
  unsigned char sensor_id;
} sensorData5;
```

### 2. Configuration Constants
- Pin assignments
- Timing values
- Radio settings
- Authentication data

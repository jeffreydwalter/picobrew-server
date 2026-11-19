/* Compile/link validation entry only; this is not brewer firmware.
 * It intentionally exercises real upstream classes, constructors and runtime.
 */
#include <Energia.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <SLFS.h>
#include <SPI.h>
#include <math.h>

static WiFiClient validation_client;
static IPAddress validation_address(8, 8, 8, 8);
static volatile double validation_math_input = 2.0;
volatile double sdk_validation_math_output;
volatile unsigned long sdk_validation_observations[6];

void setup()
{
    // Select a real SPIClass implementation without starting a bus transaction.
    SPI.setBitOrder(MSBFIRST);
    sdk_validation_observations[0] = validation_address[0];
    sdk_validation_observations[1] = WiFi.status();
    sdk_validation_observations[2] = validation_client.status();
    sdk_validation_observations[3] = SerFlash.available();
    sdk_validation_math_output = log(validation_math_input);
}

void loop()
{
    sdk_validation_observations[4] = millis();
    sdk_validation_observations[5]++;
}

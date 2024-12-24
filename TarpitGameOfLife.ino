#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>

#include "Inkplate.h"


WiFiMulti wifiMulti;
Inkplate display(INKPLATE_1BIT);

boolean oldboard[200][150] = {};
boolean board[200][150] = {};

int maxx = 200;
int maxy = 150;
int loopcounter = 0;


void setup(void)
{
  for (int x = 0; x < maxx; x++) { // Start out the board with all zeroes so that any data on the screen got there via the tarpit
    for (int y = 0; y < maxy; y++) {
      board[x][y] = 0;
    }
  }

  display.begin();
  display.clearDisplay();
  display.display();

  wifiMulti.addAP("WIFISSID", "WIFIPASSWORD"); //REPLACE these with the Wi-Fi that the E-ink screen will use to connect to the Internet
}


void loop(void)
{

  // print board, each Game of Life cell is 4x4 e-ink pixels
  for (int x = 0; x < maxx; x++) {
    for (int y = 0; y < maxy; y++) {
      if (board[x][y]) {
        display.fillRect(x * 4, y * 4, 4, 4, BLACK);
      } else {
        display.fillRect(x * 4, y * 4, 4, 4, WHITE);
      }
      oldboard[x][y] = board[x][y];
    }
  }

  // do a full e-ink screen refresh once a day (this will almost certainly drift throughout the day but I've never noticed it)
  if (loopcounter == 0) {
    display.display();
  } else {
    // Otherwise do a partial refresh, which is what we want pretty much all the time
    display.partialUpdate();
  }

  // update Game of Life board for the next iteration
  for (int x = 0; x < maxx; x++) {
    for (int y = 0; y < maxy; y++) { // count the number of neighbours that are 'alive'
      int count = 0;
      for (int nx = -1; nx <= 1; nx++) {
        for (int ny = -1; ny <= 1; ny++) {
          if (nx != 0 || ny != 0) {
            count = count + (int)oldboard[(x + nx + maxx) % maxx][(y + ny + maxy) % maxy];
          }
        }
      }

      // Game of Life rules, set the next iteration according to the number of neighbours alive
      if (count == 3 || (count == 2 && oldboard[x][y] == 1)) {
        board[x][y] = 1;
      } else {
        board[x][y] = 0;
      }
    }
  }

  // every 120 iterations, check the tarpit website for more data
  loopcounter = (loopcounter + 1) % 86400;
  
  if (loopcounter % 120 == 1) { // Double check that the WiFi is connected 
      if ((WiFi.status() == WL_CONNECTED)) {
        delay(500);
      } else {
        wifiMulti.run();
      }

    
  } else if (loopcounter % 120 == 2) {
    if ((WiFi.status() == WL_CONNECTED)) {

      HTTPClient http;
      http.begin("http://123.123.123.123:2383/CGoL"); //REPLACE ME with your tarpit's IP address as seen from the POV of the Inkplate board

      int httpCode = http.GET();
      if (httpCode > 0) {
        String payload = http.getString();
        int payloadLength = payload.length();
        if (payloadLength > 0) {

          int x = payload.charAt(0) * maxx / 256; // First two octets of the attacker's IP converted into x/y coordinates
          int y = payload.charAt(1) * maxy / 256;
          int mx = 0;
          int my = 0;
          int num = 0;
          for (int pos = 2; pos < payloadLength; pos++) { // For the rest of the data in the message...
            mx = pos % 8;
            my = 0;
            num = payload.charAt(pos); // take a byte ...
            while (num > 0) { // and for each bit in that byte...
              if (num % 2) { // invert the GoL cell in position x+mx, y+my if the bit is 1
                board[(x + mx) % maxx][(y + my) % maxy] = 1 - board[(x + mx) % maxx][(y + my) % maxy];
              }
              num = num / 2;
              my += 1; //mx and my go from 0 to 7
            }
          }
        }
      }
      http.end();
    }
  } else {
    delay(500);
  }


}

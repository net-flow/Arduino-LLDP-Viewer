#include <SoftwareSerial.h>
#include <SPI.h>         
#include <Ethernet.h>
#include <utility/w5100.h>
#include <Stdio.h>

#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

SOCKET s; // the socket that will be openend in RAW mode
byte rbuf[380+14]; //receive buffer (was 1500+14, but costs way too much SRAM)
int rbuflen; // length of data to receive
bool byte_array_contains(const byte a[], unsigned int offset, const byte b[], unsigned int length);
char TLVTYPE[3];
char TLVLENGTH[3];



#define MAC_LENGTH 6

byte mymac[] = {0xC0, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
byte lldp_mac[] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e};  // dst mac for lldp traffic, we look for this
byte bmac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


void setup() {
  W5100.init();
  W5100.writeSnMR(s, SnMR::MACRAW);
  W5100.execCmdSn(s, Sock_OPEN);
  Serial.begin(115200);
  
  lcd.begin(16, 2);
 lcd.print("Waiting for LLDP");

}

void loop() {

  rbuflen = W5100.getRXReceivedSize(s);
    if(rbuflen>0) {
    if(rbuflen > sizeof(rbuf))
    rbuflen = sizeof(rbuf);
    W5100.recv_data_processing(s, rbuf, rbuflen);
    W5100.execCmdSn(s, Sock_RECV);
    
    } 

    lldpcheck(); 

  
  
/*  
uint8_t buttons = lcd.readButtons();
String testing = "Cisco";

  if (buttons) {
    lcd.clear();
    lcd.setCursor(0,0);
    if (buttons & BUTTON_UP) {
        }
    if (buttons & BUTTON_SELECT) {
      lcd.print(testing);
    }
    }
*/


}



void lldpcheck() {
   unsigned int rbufIndex = 2;
   if(byte_array_contains(rbuf, rbufIndex, lldp_mac, sizeof(lldp_mac))) {

    //Serial.print(F("Received "));
    //Serial.println(rbuflen, DEC);
    if (rbuflen == 0){
      //Serial.println("returning, 0 length");
      return;
   }
    char tmp[rbuflen*2+1];
      byte first;
      byte second;
      for (int i=0; i<rbuflen; i++) {
            first = (rbuf[i] >> 4) & 0x0f;
            second = rbuf[i] & 0x0f;
            tmp[i*2] = first+48;
            tmp[i*2+1] = second+48;
            if (first > 9) tmp[i*2] += 39;
            if (second > 9) tmp[i*2+1] += 39;
      }
      tmp[rbuflen*2] = 0;
      //Serial.println(tmp);
      //Serial.println("Data ");
      
      
      //main while loop for data extraction 
      int y = 0;
      int x = 32;//start reading at the first TLV value so we start at 32.  We don't care about the source/dest mac.
      while(y < 10){


      //find TLV Data Type
      for (int i=0; i<2; i++) {
        TLVTYPE[i] = tmp[x++];
      }
      
      TLVTYPE[3]='\0';  // required null pointer set  
    

      //find TLV lenghts
      for (int i=0; i<2; i++) {
          TLVLENGTH[i] = tmp[x++];
      }


      
      //this converts the Hex like value in TLVLENGTH to an int (xx) so we can use it for math
      int xx=0; 
      sscanf(TLVLENGTH, "%x %x", &xx);  
     // Serial.print("TLVLENGHT as int ");
     // Serial.println(xx);
      //2x for hex like values for spaces to move
      xx=xx*2;
      //odd storage for character data array
     // Serial.print("TLVL times 2 is: ");
     // Serial.println(xx);
        
           
      //pull TLV DATA for the required length
      
      //char* TLVDATA = malloc((xx+1) * sizeof(char));
      //char* TLVDATA = (char*)malloc((xx+1) * sizeof(char));
      
      if (xx<100){  //skips overly large characters that we don't have enough memory for
        char TLVDATA[xx+1];

        for (int i=0; i<xx; i++) {
          TLVDATA[i] = tmp[x++];
        }
        TLVDATA[xx]='\0';
       // Serial.println("TLVDATA char");
        //Serial.println(TLVDATA);   

      //convert TLVTYPE TO INT FOR PROCESSING
      int xx1; 
      sscanf(TLVTYPE, "%x %x", &xx1); 
     // Serial.println("TLVTYPE IN INT format");
     // Serial.println(xx1);
      
      if(xx1==0){//end of packet!
        y=20; //exit the loop.  This should look for the end of sequence in the datastream, but we just count to 20 for now
      return;
      }

      if(xx1==254){ //vlan id
            //Serial.println("VLAN if statement match");
      int vlanhold=9;  //start at 9
      char vlan[5];
        for (int i=0; i<4; i++){
          vlan[i]=TLVDATA[vlanhold];
          vlanhold++;
        }
        vlan[5]='\0'; //null pointer
     //         Serial.println("VLAN char:");
     //         Serial.println(vlan);
        int vlanxx; 
        sscanf(vlan, "%x %x", &vlanxx); 
        if ((vlanxx > 0) && ( vlanxx < 3000)){
              //Serial.println("this is VLAN:");
              //Serial.println(vlanxx);
              //lcd.setCursor(0,0);
              //lcd.print("V:");            
              lcd.setCursor(0,0);
              lcd.print(vlanxx);
        }
        
              
              //lcd.setCursor(0,2);
              //lcd.clear();
              //lcd.print(vlanxx);
                     
                //convert to char
      }
      
      if(((xx1==4)||(xx1==10)||(xx1==2066))){// ascii conversation matches 
      
     // Serial.println("ASCII if statement match");
      
      //This pulls the data apart and converts it back into ASCII
      //We have to take the char bytes out, add them together in a char array, treat that like a hex, then find the ascii value 
             int zz=0;  //main loop for ASCII counter
             int varhold=0;  //holder for each word
             char asciiname[xx]; //final placeholder for ASCII name
             //char hexconverstion[3]; //holder to combine the values together to make the HEX
             char tempchar;  //another holders
             char* hexconverstion = (char*)malloc(3);
           
             while(zz < xx/2){
               for (int i=0; i<2; i++){
                 hexconverstion[i]=TLVDATA[varhold]; 
             // Serial.println("###START OF ASCII WHILE LOOP####");
             // Serial.println("I value is:");
               //  Serial.println(i);
                // Serial.println("zz value is:");
                 //Serial.println(zz);

                 //Serial.println("TLVDATA is is:");
                 //Serial.println(TLVDATA[varhold]);
                 //Serial.println("###END OF ASCII WHILE LOOP####");
                 varhold=varhold++;
                   hexconverstion[3]='\0'; //null pointer
               }
               hexconverstion[3]='\0'; //null pointer
               //convert this new single byte into a hex value and display the ASCII VALUE
               //    Serial.println("hexconverstion non-hex is:");
               //  Serial.println(hexconverstion);
               //  Serial.println(hexconverstion[0]);
               //  Serial.println(hexconverstion[1]);
              //   Serial.println(hexconverstion[3]);
                int xx2; 
                sscanf(hexconverstion, "%x %x", &xx2); 
              //  Serial.println("this is hexconverstion HEX in int format");
             //   Serial.println(xx2);
                //convert to char
                tempchar = (char)xx2;
              //  Serial.println("this is tempchar FINAL LETTER");
             //   Serial.println(tempchar);
               
                if (xx1==4){
                //Interface name mathc a display fix
                asciiname[zz-1]= tempchar; // junk byte in 
                }else{
                  //normal switch name
                asciiname[zz]= tempchar;
                }
 
                free(hexconverstion);
                zz=zz++;
               
             }
             if (xx1==4){
                //Interface name mathc a display fix
              asciiname[zz-1]= '\0';
                }else{
              //normal switch name
              asciiname[zz]= '\0';
                }

              
              //put the interface and switch name in the right place
              if (xx1==4){
                //this is the interface name

                lcd.clear();
                lcd.setCursor(8,0);
                lcd.print(asciiname);
                
                //binks once to show the user we have a new lldp packet 
                //we blink here becuase this is the first event we process  
                lcd.setBacklight(0x2);
                delay(200);
                lcd.setBacklight(0x3);
                
              }else{
                //this is the switchport name
                lcd.setCursor(0,1);
                lcd.print(asciiname);
              }

      
     }//end of first if statement
      
      
      }else{
    //Serial.println("DID NOT CREATE TLVDATA char");
        x=x+xx;
      }

     y=y++;  //add to main while loop counter
     
        }
  //main while stop
  
  }
}


//stolen from CDP sniffer code.  This triggers on the correct mac address 
bool byte_array_contains(const byte a[], unsigned int offset, const byte b[], unsigned int length) {
  for(unsigned int i=offset, j=0; j<length; ++i, ++j) {
    if(a[i] != b[j]) return false;
  }
  
  return true;
}


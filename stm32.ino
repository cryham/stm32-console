
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


//  const, pins
//----------------------------------------
//  oled, hardware SPI
//  MOSI PA7  CLK PA5  
#define OLED_DC   PB0
#define OLED_CS   PB1
#define OLED_RST  PB11  //not used

#define WIDTH  128
#define HEIGHT  64

Adafruit_SSD1306 display(WIDTH, HEIGHT, &SPI, OLED_DC, OLED_RST, OLED_CS);


//  keys  ----
//  K0 led left  K1 led down  X pwr  K2 led top  L lcd/sw  K3 usb lap
//    K4 spkr bath  K5 headphones  K6 dec -  K7 inc + bright

//  K0 K1 X` K2 L` K3
//    K4 K5  K7- K6+
const uint8_t keys = 8, pwms = 3;
#define  NONE  255
#define  pwmMax  255  //4095?
const uint8_t keyPin[keys]    = { PA1 , PA2 , PB9 ,  PA0 , PC14, PA4 ,  PA3 , PC15 };  // right free: PB10, PA6?, PC13 led
const uint8_t outPin[keys]    = { PA8 , PA9 , PA10,  PB12, PB14, PB13,  NONE, NONE };  // left free: PA11,12, PA15, PB3..9
const uint8_t outPwmPin[keys] = {    0,    1,    2,  NONE, NONE, NONE,  NONE, NONE };  // id to pwmVal

const int debounce = 7;  // loops not ms, each slope
const int osdMax = 120;  // loops


//  var
//--------------------
uint16_t cnt = 0;
uint16_t pwmVal[pwms] = { 120, 120, 12 };
uint8_t pwmPin = 0,  // cur for + -
    key0[keys] = {0,}, key1[keys] = {0,},  // cnt
    keySt[keys] = {1,}, keyOld[keys] = {1,},  // state
    keyVal[keys] = {0,};  // toggle value


//  osd text
//--------------------
int osdCnt = osdMax;  char osdTxt[32] = "Hello :)";
char osdTxtOn[8] = {0}, osdTxtOff[8] = {0};

void Osd(const char* txt, int on = -1)
{
    osdCnt = osdMax;
    strcpy(osdTxt, txt);
    strcpy(osdTxtOn,  on == 1 ? "On" : "");
    strcpy(osdTxtOff, on == 0 ? "off" : "");
}

void SetPwm()
{
    int pwmId = outPwmPin[pwmPin];
    if (pwmId != NONE && pwmId < pwms)
        analogWrite(outPin[pwmPin], keyVal[pwmPin] ? pwmVal[pwmId] : 0);
}


//  config
//----------------------------------------
void setup()
{
    //  oled
    display.begin(SSD1306_SWITCHCAPVCC);
    display.dim(1);
    
    //  keys pins
    for (int i=0; i < keys; ++i)
    {
        key0[i] = 0;  key1[i] = 0;  
        keySt[i] = 1;  keyOld[i] = 1;
        keyVal[i] = 0;
        if (keyPin[i] != NONE)
            pinMode(keyPin[i], INPUT_PULLUP);
    }
    //  out pins
    for (int i=0; i < keys; ++i)
        if (outPin[i] != NONE)
            pinMode(outPin[i], OUTPUT);

    //analogWriteResolution(12);  //?
}


//  main
//------------------------------------------------------------
void loop()
{
    //  keys read
    for (int i=0; i < keys; ++i)
    if (keyPin[i] != NONE)
    {
        //  debounce
        int st = digitalRead(keyPin[i]);
        if (st && !keySt[i])
        {   key1[i]++;
            if (key1[i] > debounce)
            {   key1[i] = 0;  key0[i] = 0;  keySt[i] = 1;
        }   }
        if (!st && keySt[i])
        {   key0[i]++;
            if (key0[i] > debounce)
            {   key0[i] = 0;  key1[i] = 0;  keySt[i] = 0;
        }   }

        //  toggle keys, pressed  --------------------
        if (keyOld[i] && !keySt[i] && i < 6)  // par
        {
            keyVal[i] = 1 - keyVal[i];
            if (outPin[i] != NONE)
            {
                if (outPwmPin[i] == NONE)
                    digitalWrite(outPin[i], keyVal[i] ? HIGH : LOW);
                else
                {
                    pwmPin = i;  // set cur for + -
                    SetPwm();
            }   }
            
            switch (i)  // osd info
            {
            case 0:  Osd("*1 Left <",  keyVal[i]);  break;
            case 1:  Osd("*2 Down v",  keyVal[i]);  break;
            case 2:  Osd("*3 Top ^",   keyVal[i]);  break;
            case 3:  Osd("USB laptop", keyVal[i]);  break;
            case 4:  Osd("Spkr: Bath", keyVal[i]);  break;
            case 5:  Osd("Spkr: Head", keyVal[i]);  break;
        }   }


        //  continuous keys  --------------------
        int pwmId = outPwmPin[pwmPin];
        if (pwmId != NONE && pwmId < pwms)
        {
            uint16_t& p = pwmVal[pwmId];
            int step = max(1, 6 - p / 8);  // exp
            char s[32];

            if (i == 7 && !keySt[i])
            {   //  dec -  bright
                if (cnt % step == 0)
                    if (p > 0)  --p;
                sprintf(s, "Val%d %d", pwmId+1, p);  Osd(s);
                SetPwm();
            }
            if (i == 6 && !keySt[i])
            {   //  inc +
                if (cnt % step == 0)
                    if (p < pwmMax)  ++p;
                sprintf(s, "Val%d %d", pwmId+1, p);  Osd(s);
                SetPwm();
            }
        }
        keyOld[i] = keySt[i];
    }


    //  draw
    //--------------------
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
#if 1
    display.setTextSize(2);
    if (osdCnt > 0)
    {   --osdCnt;
        display.setCursor(64, 0);  display.print(osdTxtOn);   // orng
        display.setCursor(64,16);  display.print(osdTxtOff);  // blue
        display.setCursor( 0,40);  display.print(osdTxt);  // info
    }
#else  // test keys full
    display.setTextSize(1);
    display.setCursor(0,16);
    for (int i=0; i < 4; ++i)
    {
        display.print(keyVal[i]);  display.print(' ');
        display.print(keySt[i]);   display.print(' ');
        display.print(key0[i]);    display.print(' ');
        display.print(key1[i]);    display.println();
    }
#endif

    display.display();  ++cnt;
}

# include <avr/delay.h>
# include <string.h>

#define TXPIN               7
#define MAX_CHANNEL         4
#define MAX_OUTLET          3
#define GAP_US          10000
#define TXREPS             100
#define TX_CMD_LEN         16
#define TX_CMD_OUTLETS      6
#define TX_CMD_CHANNELS    12
#define USER_CMD_LEN       16

int pulse_lengths[2][2] = {
  {
    2000, /* SHORT LOW, in µs */
     500  /* SHORT HIGH, in µs */
  },
  {
     800, /* LONG LOW, in µs */
    1700  /* LONG HIGH, in µs */
  }
};

#define PULSE_LOW   0
#define PULSE_HIGH  1
#define PULSE_SHORT 0
#define PULSE_LONG  1

byte rev_order[MAX_OUTLET] = { 1, 1, 0 };
byte outlet_offsets[MAX_OUTLET] = { 2, 4, 0 };

char tx_cmd[TX_CMD_LEN + 1];
char user_cmd[USER_CMD_LEN + 1];
int  user_idx = 0;

void real_tx_command() {
  for (int rep = 0; rep < TXREPS; rep++) {
    for (int i = 0; i < TX_CMD_LEN; i++) {
      int pulse = (tx_cmd[i] == 'L' || tx_cmd[i] == 'l')
        ? PULSE_LONG
        : PULSE_SHORT; /* default to short */
    
      digitalWrite(TXPIN, HIGH);
      delayMicroseconds(pulse_lengths[pulse][PULSE_HIGH]);
      digitalWrite(TXPIN, LOW);
      delayMicroseconds(pulse_lengths[pulse][PULSE_LOW]);    
    }
  
    delayMicroseconds(GAP_US);
  }
}

void tx_command() {
  tx_cmd[TX_CMD_LEN] = '\0';
  Serial.println("SENDING");
  Serial.println(tx_cmd);
  real_tx_command();
  Serial.println("DONE");
}

void send_state(int channel, int outlet, int state) {
  char *header  = "SLLSLS";
  char *outlets = "SSSSSS";
  char *channels = "SSSS";

  if (rev_order[outlet]) {
    state = !state;
  }

  strncpy(tx_cmd, "SLLSLSSSSSSSSSSS", TX_CMD_LEN);
  
  tx_cmd[TX_CMD_OUTLETS + outlet_offsets[outlet] + state] = 'L';
  tx_cmd[TX_CMD_CHANNELS + channel] = 'L';
  tx_command();
}

void process_command() {
  int i;
  int channel = -1;
  int outlet = -1;
  int state = -1;

  Serial.print("> ");
  Serial.println(user_cmd);

  if (user_cmd[0] - 'A' < 0 || user_cmd[0] - 'A' >= MAX_CHANNEL) {
    Serial.println("ERROR: Invalid Channel");
  } else {
    channel = user_cmd[0] - 'A'; /* zero-index channels */
    
    if (user_cmd[1] - '1' < 0 || user_cmd[1] - '1' >= MAX_OUTLET) {
      Serial.println("ERROR: Invalid Outlet");
    } else {
      outlet = user_cmd[1] - '1'; /* zero-index outlets */
      
      for (i = 2; i < USER_CMD_LEN && (user_cmd[i] == '\t' || user_cmd[i] == ' '); i++)
        ;

      if (strcasecmp(&user_cmd[i], "on") == 0) {
        state = 1;
      } else if (strcasecmp(&user_cmd[i], "off") == 0) {
        state = 0;
      }
      
      if (state != -1) {
        send_state(channel, outlet, state);
      } else {
        Serial.println("ERROR: Invalid State");
      }
    }
  }
  user_idx = 0;
  user_cmd[user_idx] = '\0';  
}


void setup() {
  pinMode(TXPIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("READY");
}

void loop() {
  int ch;
  while (Serial.available() > 0 && user_idx < USER_CMD_LEN) {
    ch = Serial.read();
    if (ch == 10 || ch == 13) {
      user_cmd[user_idx++] = '\0';
      process_command();
    } else {
      user_cmd[user_idx++] = ch;
    }
  }
  if (user_idx >= USER_CMD_LEN) {
    Serial.println("ERROR: Command Too Long");
    user_idx = 0;
    user_cmd[user_idx] = '\0';
  }
}



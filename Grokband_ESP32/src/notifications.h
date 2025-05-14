#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

void notification_init();
void notification_alert_message(bool long_buzz = false); // LED blink + vibration
void notification_led_on();
void notification_led_off();
void notification_vibrate(int duration_ms);

#endif // NOTIFICATIONS_H
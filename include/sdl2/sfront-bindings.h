#ifndef TEO_SFRONT_BINDINGS_H
#define TEO_SFRONT_BINDINGS_H

#ifdef PLATFORM_OGXBOX
#define PANEL_TOGGLE_BUTTON 7 //start

#define VKB_HAXIS 3 //Right thumb H
#define VKB_VAXIS 4 //Right thumb V
#define VKB_BTN 0 //Button A 
#define VKB_TOGGLE_BTN 9 //Right thumb click

#define VJOY_VERTICAL_AXIS 1 //Left thumb V
#define VJOY_HORIZONTAL_AXIS 0 //Left thumb H
#define VJOY_A_BOUND_BTN 0 //Button A
#define VJOY_B_BOUND_BTN 1 //Button B

#define JMOUSE_VERTICAL_AXIS 4 //Right thumb H
#define JMOUSE_HORIZONTAL_AXIS 3 //Right thumb V
#define JMOUSE_BUTTON_LEFT 4 //White btn 
#define JMOUSE_BUTTON_RIGHT 6 //Black btn
#define JMOUSE_INVERTED_VA 1 
#define JMOUSE_INVERTED_HA 1


#else
#define PANEL_TOGGLE_BUTTON 9

/*Axis/button that controls the virtual keyboard*/
#define VKB_HAXIS 2
#define VKB_VAXIS 3
#define VKB_BTN 4
#define VKB_TOGGLE_BTN 11

/*Axis/buttons that are bound to the emulated TO8 joystick*/
#define VJOY_VERTICAL_AXIS 1
#define VJOY_HORIZONTAL_AXIS 0
#define VJOY_A_BOUND_BTN 0
#define VJOY_B_BOUND_BTN 1

/* Axis/buttons that are bound to the TO8 mouse/lightpen when
 * using the host joystick to control the TO8 Mouse
 */
#define JMOUSE_VERTICAL_AXIS 3
#define JMOUSE_HORIZONTAL_AXIS 2
#define JMOUSE_BUTTON_LEFT 4
#define JMOUSE_BUTTON_RIGHT 5
#define JMOUSE_INVERTED_VA 0 
#define JMOUSE_INVERTED_HA 0

#endif //PLATFORM_OGXBOX


#endif //TEO_SFRONT_BINDINGS_H

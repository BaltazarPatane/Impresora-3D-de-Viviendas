/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "env_validate.h"

// Custom flags and defines for the build
//#define BOARD_CUSTOM_BUILD_FLAGS -D__FOO__

//BALTA
//Nombre de la placa para mí?
#ifndef BOARD_INFO_NAME
  #define BOARD_INFO_NAME "R3_Shield"
#endif

//BALTA
// Motores en el plano X
//
#define X_STEP_PIN                            54  // (A0)
#define X_DIR_PIN                             55  // (A1)
#define X_ENABLE_PIN                          56  // (A2)

#define X2_STEP_PIN                           58  // (A4)
#define X2_DIR_PIN                            57  // (A3)
#define X2_ENABLE_PIN                         59  // (A5)

// Mismos movimientos pero distintos pines (double carriage)
// Tengo anotado que tienen sentidos inversos

//BALTA
// Motor en el plano Y
//
#define Y_STEP_PIN                            60  // (A6)
#define Y_DIR_PIN                             61  // (A7)
#define Y_ENABLE_PIN                          63  // (A9)


//BALTA
// Pistones hidráulicos en el plano Z

// Los siguientes son los pines predeterminados del Marlin
// Quizás los pueda aprovechar para no crear nuevos (no me servía ni me convenía usarlos)
// Al final no los uso pero el Marlin ahora reconoce el eje

#define Z_STEP_PIN                            28  //
#define Z_DIR_PIN                             28  //
#define Z_ENABLE_PIN                          28  //

#define Z1_DOWN_PIN                            42  // (D42)
#define Z1_UP_PIN                              44  // (D44)
#define Z2_DOWN_PIN                            46  // (D46)
#define Z2_UP_PIN                              48  // (D48)

// Agregar si se tiene un extrusor
// Creo que tenemos pero necesitamos los pines

///////////////////////////
#define E0_STEP_PIN                            28  //
#define E0_DIR_PIN                             28  //
#define E0_ENABLE_PIN                          28  //
//////////////////////////


//BALTA
// Limitar movimientos
//

#define X_MIN_PIN                              28
#define X_MAX_PIN                              28

#define Y_MIN_PIN                              8  // (D8)
#define Y_MAX_PIN                              9  // (D9)

// BALTA (AGREGAR)
// Este todavía no lo implementé
#define Z_MIN_PIN                              10  // (D10)
#define Z_MAX_PIN                              28

#define Z_BTN_SUBIR                            30
#define Z_BTN_BAJAR                            32
#define X_BTN_SUBIR                            34
#define X_BTN_BAJAR                            10
#define Y_BTN_SUBIR                            40
#define Y_BTN_BAJAR                            38

//
// Display (los pines en 5 son del display)
//
#ifdef REPRAP_DISCOUNT_FULL_GRAPHIC_SMART_CONTROLLER 
  #define LCD_PINS_RS      16  //Selección de regsitros (comando o datos)
  #define LCD_PINS_EN      17  //Pin de enable para transferencia
  #define LCD_PINS_D4      23  //Línes de datos en paralelo
  //#define LCD_PINS_D5      5
  //#define LCD_PINS_D6      5
  //#define LCD_PINS_D7      5
  #define ST7920_CLK_PIN   LCD_PINS_D4  //Reloj serial para el controlador ST7920
  #define ST7920_DAT_PIN   LCD_PINS_EN  //Línea de datos serial hacia el ST7920
  #define ST7920_CS_PIN    LCD_PINS_RS  //Chip Select: activa la pantalla para comunicarse
  //#define BOARD_ST7920_DELAY_1 125  //Valores de retardo (en nanosegundos) usados para ajustar el timing entre cambios de pines (clock, data, enable)
  //#define BOARD_ST7920_DELAY_2 125
  //#define BOARD_ST7920_DELAY_3 125
  #define BTN_ENC        35
  #define BTN_EN1        33
  #define BTN_EN2        31
  #define BEEPER_PIN     37
  #ifdef SDSUPPORT
    #define SD_DETECT_PIN 49
    #define SDSS 53
    #define SDPOWER_PIN -1
    #define SD_DETECT_STATE (HIGH)
  #endif
#endif

//
// IMPORTANTE: SI QUIERO AGREGAR PINES TENGO QUE SOBREESCRIBIR LOS QUE YA ESTÁN EN USO Y ADEMÁS HACERLE EL INIT A LOS NUEVOS
//

// Todos los pines en 28 son porque no los usamos pero Marlin los pide por seguridad
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

#include "../gcode.h"
#include "../../module/motion.h"
#include "../../module/planner.h"

#include "../../MarlinCore.h"

#if ALL(FWRETRACT, FWRETRACT_AUTORETRACT)
  #include "../../feature/fwretract.h"
#endif

#include "../../sd/cardreader.h"

#if ENABLED(NANODLP_Z_SYNC)
  #include "../../module/planner.h"
#endif

extern xyze_pos_t destination;

#if ENABLED(VARIABLE_G0_FEEDRATE)
  feedRate_t fast_move_feedrate = MMM_TO_MMS(G0_FEEDRATE);
#endif


// Tabla generada desde MATLAB
// Correccion X en funcion de Z
// Z en mm, X en mm
static const int N_TABLA_XZ = 50;
static const float Z_TABLE[N_TABLA_XZ] = {
  0.0000f,
  66.3394f,
  132.5068f,
  196.8724f,
  262.6313f,
  328.1487f,
  393.4010f,
  458.3651f,
  521.4448f,
  585.7714f,
  649.7409f,
  713.3304f,
  776.5171f,
  837.7526f,
  900.0767f,
  961.9305f,
  1023.2918f,
  1084.1382f,
  1142.9833f,
  1202.7477f,
  1261.9320f,
  1320.5144f,
  1378.4735f,
  1434.3979f,
  1491.0632f,
  1547.0423f,
  1602.3146f,
  1655.5380f,
  1709.3537f,
  1762.4022f,
  1814.6636f,
  1866.1182f,
  1915.5220f,
  1965.3262f,
  2014.2669f,
  2062.3254f,
  2109.4835f,
  2154.6062f,
  2199.9325f,
  2244.3055f,
  2287.7080f,
  2330.1233f,
  2370.5368f,
  2410.9533f,
  2450.3344f,
  2488.6644f,
  2525.9282f,
  2561.2415f,
  2596.3557f,
  2630.3607f
};
static const float X_TABLE[N_TABLA_XZ] = {
  0.0000f,
  7.0509f,
  15.3822f,
  24.7413f,
  35.5926f,
  47.7136f,
  61.0998f,
  75.7462f,
  91.2447f,
  108.3643f,
  126.7264f,
  146.3242f,
  167.1503f,
  188.6449f,
  211.8745f,
  236.3079f,
  261.9361f,
  288.7495f,
  316.0415f,
  345.1664f,
  375.4454f,
  406.8671f,
  439.4198f,
  472.2569f,
  507.0079f,
  542.8525f,
  579.7773f,
  616.8293f,
  655.8472f,
  695.9031f,
  736.9820f,
  779.0685f,
  821.0846f,
  865.1154f,
  910.1058f,
  956.0393f,
  1002.8985f,
  1049.4904f,
  1098.1269f,
  1147.6361f,
  1197.9995f,
  1249.1983f,
  1299.9352f,
  1352.7279f,
  1406.2982f,
  1460.6261f,
  1515.6912f,
  1570.1042f,
  1626.5651f,
  1683.7013f
};
float correccionX_desdeZ(float z_mm) {
  if (z_mm <= Z_TABLE[0]) return X_TABLE[0];
  if (z_mm >= Z_TABLE[N_TABLA_XZ - 1]) return X_TABLE[N_TABLA_XZ - 1];
  for (int i = 0; i < N_TABLA_XZ - 1; i++) {
    if (z_mm >= Z_TABLE[i] && z_mm <= Z_TABLE[i + 1]) {
      float t = (z_mm - Z_TABLE[i]) / (Z_TABLE[i + 1] - Z_TABLE[i]);
      return X_TABLE[i] + t * (X_TABLE[i + 1] - X_TABLE[i]);
    }
  }
  return 0.0f;
}

//static float z_anterior = 0.0f;

/**
 * G0, G1: Coordinated movement of X Y Z E axes
 */

  // BALTA (PASO 1)
  // ACÁ ARRANCA CON EL MOVIMIENTO

  /* G0_G1 (FALSE):
  Es la capa de interfaz entre G-code y subsistema de movimiento: hace checks,
  adapta feedrates y decide ruta (e.g., si hay autoretract convertirá ciertos E-only en retracts).
  Si todo eso estuviera mezclado en el planner/stepper sería difícil mantener y testar.
  Además maneja muchos features opcionales (G0_FEEDRATE, FWRETRACT, NANODLP_Z_SYNC...)
  que no deberían enturbiar la lógica de planificación.

  Mantener aquí sólo parsing / decisiones de alto nivel.
  Evitar colocar lógicas de temporización o de motor (esas pertenecen al planner/stepper).
  Asegúrate de restaurar el feedrate_mm_s correctamente. */

void GcodeSuite::G0_G1(TERN_(HAS_FAST_MOVES, const bool fast_move/*=false*/)) {
  if (!MOTION_CONDITIONS) return;

  TERN_(FULL_REPORT_TO_HOST_FEATURE, set_and_report_grblstate(M_RUNNING));

  #ifdef G0_FEEDRATE
    feedRate_t old_feedrate;
    #if ENABLED(VARIABLE_G0_FEEDRATE)
      if (fast_move) {
        old_feedrate = feedrate_mm_s;             // Back up the (old) motion mode feedrate
        feedrate_mm_s = fast_move_feedrate;       // Get G0 feedrate from last usage
      }
    #endif
  #endif

  get_destination_from_command();                 // Get X Y [Z[I[J[K]]]] [E] F (and set cutter power);

  const bool hay_x = parser.seenval('X');
  const bool hay_z = parser.seenval('Z');

  #ifdef G0_FEEDRATE
    if (fast_move) {
      #if ENABLED(VARIABLE_G0_FEEDRATE)
        fast_move_feedrate = feedrate_mm_s;       // Save feedrate for the next G0
      #else
        old_feedrate = feedrate_mm_s;             // Back up the (new) motion mode feedrate
        feedrate_mm_s = MMM_TO_MMS(G0_FEEDRATE);  // Get the fixed G0 feedrate
      #endif
    }
  #endif

  #if BOTH(FWRETRACT, FWRETRACT_AUTORETRACT)

    if (MIN_AUTORETRACT <= MAX_AUTORETRACT) {
      // When M209 Autoretract is enabled, convert E-only moves to firmware retract/recover moves
      if (fwretract.autoretract_enabled && parser.seen_test('E')
        && !parser.seen(STR_AXES_MAIN)
      ) {
        const float echange = destination.e - current_position.e;
        // Is this a retract or recover move?
        if (WITHIN(ABS(echange), MIN_AUTORETRACT, MAX_AUTORETRACT) && fwretract.retracted[active_extruder] == (echange > 0.0)) {
          current_position.e = destination.e;       // Hide a G1-based retract/recover from calculations
          sync_plan_position_e();                   // AND from the planner
          return fwretract.retract(echange < 0.0);  // Firmware-based retract/recover (double-retract ignored)
        }
      }
    }

  #endif // FWRETRACT

  if (NEAR(current_position.x, destination.x) && NEAR(current_position.y, destination.y) && NEAR(current_position.z, destination.z)) return;

  if (hay_z) {
    // Drena la cola del planner
    planner.synchronize();

    // Evitar que quiera irse más arriba de lo que puede
    if (destination.z > largo_brazo) destination.z = largo_brazo;

      // Dirección de desplazamiento
      double dir_z = (destination.z > Planner::destino_local_Z) ? 1.0 : -1.0;
      Planner::destino_local_Z = destination.z;

      Planner::Z_BUSY_1 = true;
      Planner::Z_BUSY_2 = true;

      // Se activan las electroválvulas correspondientes y se guarda la dirección de movimiento
      if (dir_z > 0) {
        extDigitalWrite(Z1_UP_PIN, 255);
        hal.set_pwm_duty(Z1_UP_PIN, 255);
        extDigitalWrite(Z1_DOWN_PIN, 0);
        hal.set_pwm_duty(Z1_DOWN_PIN, 0);
        extDigitalWrite(Z2_UP_PIN, 255);
        hal.set_pwm_duty(Z2_UP_PIN, 255);
        extDigitalWrite(Z2_DOWN_PIN, 0);
        hal.set_pwm_duty(Z2_DOWN_PIN, 0);
        Planner::subiendo = true;
      }
      else {
        extDigitalWrite(Z1_UP_PIN, 0);
        hal.set_pwm_duty(Z1_UP_PIN, 0);
        extDigitalWrite(Z1_DOWN_PIN, 255);
        hal.set_pwm_duty(Z1_DOWN_PIN, 255);
        extDigitalWrite(Z2_UP_PIN, 0);
        hal.set_pwm_duty(Z2_UP_PIN, 0);
        extDigitalWrite(Z2_DOWN_PIN, 255);
        hal.set_pwm_duty(Z2_DOWN_PIN, 255);
        Planner::subiendo = false;
      }

      extDigitalWrite(LED_AMARILLO, 0);
      hal.set_pwm_duty(LED_AMARILLO, 0);
      // Se espera a que el pistón llegue a su destino
      while (Planner::Z_BUSY_1 || Planner::Z_BUSY_2) {
        // BALTA
        // Leo los encoders constantemente

        if (READ(BOTON_EMERGENCIA) == HIGH) {
          extDigitalWrite(LED_ROJO, 0);
          hal.set_pwm_duty(LED_ROJO, 0);
          extDigitalWrite(LED_VERDE, 255);
          hal.set_pwm_duty(LED_VERDE, 255);
          extDigitalWrite(LED_AMARILLO, 255);
          hal.set_pwm_duty(LED_AMARILLO, 255);
          planner.quick_stop();
          Planner::Z_BUSY_1 = false;
          Planner::Z_BUSY_2 = false;
          kill();
        }

        leer_encoder();

        // BALTA
        // Analizo constantemente si el pistón llegó a su destino
        if ((Planner::subiendo) && (Planner::Z_BUSY_1)){
          if (h1_mm >= (Planner::destino_local_Z - 1.5)) {Planner::z_isr_1();}
        }
        else if (!(Planner::subiendo) && (Planner::Z_BUSY_1)) {
          if (h1_mm <= (Planner::destino_local_Z + 1.5)) {Planner::z_isr_1();}
        }

        if ((Planner::subiendo) && (Planner::Z_BUSY_2)){
          if (h2_mm >= (Planner::destino_local_Z - 1.5)) {Planner::z_isr_2();}
        }
        else if (!(Planner::subiendo) && (Planner::Z_BUSY_2)) {
          if (h2_mm <= (Planner::destino_local_Z + 1.5)) {Planner::z_isr_2();}
        }
      }

      // Se desactivan las electroválvulas
      extDigitalWrite(Z1_UP_PIN, 0);
      hal.set_pwm_duty(Z1_UP_PIN, 0);
      extDigitalWrite(Z1_DOWN_PIN, 0);
      hal.set_pwm_duty(Z1_DOWN_PIN, 0);
      extDigitalWrite(Z2_UP_PIN, 0);
      hal.set_pwm_duty(Z2_UP_PIN, 0);
      extDigitalWrite(Z2_DOWN_PIN, 0);
      hal.set_pwm_duty(Z2_DOWN_PIN, 0);

      extDigitalWrite(LED_AMARILLO, 255);
      hal.set_pwm_duty(LED_AMARILLO, 255);


    #ifdef G0_FEEDRATE
      // Restore the motion mode feedrate
      if (fast_move) feedrate_mm_s = old_feedrate;
    #endif

    // Como los pistones ya llegaron físicamente a Z,
    // actualizo la posición interna de Marlin.
    current_position.z = Planner::destino_local_Z;
    destination.z = current_position.z;
    sync_plan_position();

    double correccion_x = correccionX_desdeZ(Planner::destino_local_Z) - correccionX_desdeZ(Planner::z_anterior);

    Planner::correccion_acumulada_x += correccion_x;

    // Esta línea la podría sacar
    // Entre un comando de Z aislado y el primer comando de X siguiente, la pantalla miente
    destination.x = current_position.x + correccion_x;

    Planner::z_anterior = Planner::destino_local_Z;
  }

  // Si hay X y no es relativo, entonces se le suma la corrección acumulada de X
  if ((hay_x) && (!axis_is_relative(X_AXIS))) {
    destination.x += Planner::correccion_acumulada_x;
  }

  #if IS_SCARA
    fast_move ? prepare_fast_move_to_destination() : prepare_line_to_destination();
  #else
    prepare_line_to_destination();
  #endif

  // Hace que al pausar se hagan máximo 3 comandos
  while (planner.movesplanned() > 2) idle();

  #if ENABLED(NANODLP_Z_SYNC)
    #if ENABLED(NANODLP_ALL_AXIS)
      #define _MOVE_SYNC parser.seenval('X') || parser.seenval('Y') || parser.seenval('Z')  // For any move wait and output sync message
    #else
      #define _MOVE_SYNC parser.seenval('Z')  // Only for Z move
    #endif
    if (_MOVE_SYNC) {
      planner.synchronize();
      SERIAL_ECHOLNPGM(STR_Z_MOVE_COMP);
    }
    TERN_(FULL_REPORT_TO_HOST_FEATURE, set_and_report_grblstate(M_IDLE));
  #else
    TERN_(FULL_REPORT_TO_HOST_FEATURE, report_current_grblstate_moving());
  #endif
}

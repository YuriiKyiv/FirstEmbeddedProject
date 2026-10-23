/**
  ******************************************************************************
  * @file           : app.h
  * @brief          : Application entry points (Arduino-style setup/loop)
  ******************************************************************************
  */

/* Include guard: prevents multiple/cyclic inclusion of this header */
#ifndef __APP_H
#define __APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Called once after all peripherals are initialized */
void setup(void);

/* Called repeatedly from the main infinite loop */
void loop(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_H */

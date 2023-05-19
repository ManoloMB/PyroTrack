/** @file
 *  @brief HTS Service sample
 */

/*
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

//void temp_init(void);
//void temp_indicate(int16_t temperature);
void nox_init(void);
void nox_indicate(int16_t nox);

#ifdef __cplusplus
}
#endif
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

void hum_init(void);
void hum_indicate(int16_t humidity);

#ifdef __cplusplus
}
#endif

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : misc.c
  * @brief          :
  ******************************************************************************
  * @attention
  *
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "misc.h"

/* Defines -------------------------------------------------------------------*/

/* Macros --------------------------------------------------------------------*/

/* Types ---------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

/**
  * @brief      Progress Bar
  */
#if 1
char dispBar[60][63] =
{
  "[|                                                           ]",     //  0
  "[||                                                          ]",     //  1
  "[|||                                                         ]",     //  2
  "[||||                                                        ]",     //  3
  "[|||||                                                       ]",     //  4
  "[||||||                                                      ]",     //  5
  "[|||||||                                                     ]",     //  6
  "[||||||||                                                    ]",     //  7
  "[|||||||||                                                   ]",     //  8
  "[||||||||||                                                  ]",     //  9
  "[|||||||||||                                                 ]",     //  10
  "[||||||||||||                                                ]",     //  11
  "[|||||||||||||                                               ]",     //  12
  "[||||||||||||||                                              ]",     //  13
  "[|||||||||||||||                                             ]",     //  14
  "[||||||||||||||||                                            ]",     //  15
  "[|||||||||||||||||                                           ]",     //  16
  "[||||||||||||||||||                                          ]",     //  17
  "[|||||||||||||||||||                                         ]",     //  18
  "[||||||||||||||||||||                                        ]",     //  19
  "[|||||||||||||||||||||                                       ]",     //  20
  "[||||||||||||||||||||||                                      ]",     //  21
  "[|||||||||||||||||||||||                                     ]",     //  22
  "[||||||||||||||||||||||||                                    ]",     //  23
  "[|||||||||||||||||||||||||                                   ]",     //  24
  "[||||||||||||||||||||||||||                                  ]",     //  25
  "[|||||||||||||||||||||||||||                                 ]",     //  26
  "[||||||||||||||||||||||||||||                                ]",     //  27
  "[|||||||||||||||||||||||||||||                               ]",     //  28
  "[||||||||||||||||||||||||||||||                              ]",     //  29
  "[|||||||||||||||||||||||||||||||                             ]",     //  30
  "[||||||||||||||||||||||||||||||||                            ]",     //  31
  "[|||||||||||||||||||||||||||||||||                           ]",     //  32
  "[||||||||||||||||||||||||||||||||||                          ]",     //  33
  "[|||||||||||||||||||||||||||||||||||                         ]",     //  34
  "[||||||||||||||||||||||||||||||||||||                        ]",     //  35
  "[|||||||||||||||||||||||||||||||||||||                       ]",     //  36
  "[||||||||||||||||||||||||||||||||||||||                      ]",     //  37
  "[|||||||||||||||||||||||||||||||||||||||                     ]",     //  38
  "[||||||||||||||||||||||||||||||||||||||||                    ]",     //  39
  "[|||||||||||||||||||||||||||||||||||||||||                   ]",     //  40
  "[||||||||||||||||||||||||||||||||||||||||||                  ]",     //  41
  "[|||||||||||||||||||||||||||||||||||||||||||                 ]",     //  42
  "[||||||||||||||||||||||||||||||||||||||||||||                ]",     //  43
  "[|||||||||||||||||||||||||||||||||||||||||||||               ]",     //  44
  "[||||||||||||||||||||||||||||||||||||||||||||||              ]",     //  45
  "[|||||||||||||||||||||||||||||||||||||||||||||||             ]",     //  46
  "[||||||||||||||||||||||||||||||||||||||||||||||||            ]",     //  47
  "[|||||||||||||||||||||||||||||||||||||||||||||||||           ]",     //  48
  "[||||||||||||||||||||||||||||||||||||||||||||||||||          ]",     //  49
  "[|||||||||||||||||||||||||||||||||||||||||||||||||||         ]",     //  50
  "[||||||||||||||||||||||||||||||||||||||||||||||||||||        ]",     //  51
  "[|||||||||||||||||||||||||||||||||||||||||||||||||||||       ]",     //  52
  "[||||||||||||||||||||||||||||||||||||||||||||||||||||||      ]",     //  53
  "[|||||||||||||||||||||||||||||||||||||||||||||||||||||||     ]",     //  54
  "[||||||||||||||||||||||||||||||||||||||||||||||||||||||||    ]",     //  55
  "[|||||||||||||||||||||||||||||||||||||||||||||||||||||||||   ]",     //  56
  "[||||||||||||||||||||||||||||||||||||||||||||||||||||||||||  ]",     //  57
  "[||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| ]",     //  58
  "[||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||]"      //  59
};
#else
char dispBar[60][63] =
{
  " .                                                            ",     //  0
  " ..                                                           ",     //  1
  " ...                                                          ",     //  2
  " ....                                                         ",     //  3
  " .....                                                        ",     //  4
  " ......                                                       ",     //  5
  " .......                                                      ",     //  6
  " ........                                                     ",     //  7
  " .........                                                    ",     //  8
  " ..........                                                   ",     //  9
  " ...........                                                  ",     //  10
  " ............                                                 ",     //  11
  " .............                                                ",     //  12
  " ..............                                               ",     //  13
  " ...............                                              ",     //  14
  " ................                                             ",     //  15
  " .................                                            ",     //  16
  " ..................                                           ",     //  17
  " ...................                                          ",     //  18
  " ....................                                         ",     //  19
  " .....................                                        ",     //  20
  " ......................                                       ",     //  21
  " .......................                                      ",     //  22
  " ........................                                     ",     //  23
  " .........................                                    ",     //  24
  " ..........................                                   ",     //  25
  " ...........................                                  ",     //  26
  " ............................                                 ",     //  27
  " .............................                                ",     //  28
  " ..............................                               ",     //  29
  " ...............................                              ",     //  30
  " ................................                             ",     //  31
  " .................................                            ",     //  32
  " ..................................                           ",     //  33
  " ...................................                          ",     //  34
  " ....................................                         ",     //  35
  " .....................................                        ",     //  36
  " ......................................                       ",     //  37
  " .......................................                      ",     //  38
  " ........................................                     ",     //  39
  " .........................................                    ",     //  40
  " ..........................................                   ",     //  41
  " ...........................................                  ",     //  42
  " ............................................                 ",     //  43
  " .............................................                ",     //  44
  " ..............................................               ",     //  45
  " ...............................................              ",     //  46
  " ................................................             ",     //  47
  " .................................................            ",     //  48
  " ..................................................           ",     //  49
  " ...................................................          ",     //  50
  " ....................................................         ",     //  51
  " .....................................................        ",     //  52
  " ......................................................       ",     //  53
  " .......................................................      ",     //  54
  " ........................................................     ",     //  55
  " .........................................................    ",     //  56
  " ..........................................................   ",     //  57
  " ...........................................................  ",     //  58
  " ............................................................ "      //  59
};
#endif


/* Function prototypes -------------------------------------------------------*/

/* User code -----------------------------------------------------------------*/

/**
  * @brief      brief
  * @param      param
  * @return     return
  * @note       note
  */

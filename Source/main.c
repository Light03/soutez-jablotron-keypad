/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32l0xx_hal.h"

/* USER CODE BEGIN Includes */


#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>



/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim21;
TIM_HandleTypeDef htim22;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
char CurrentNormalSequence[32];

int CurrentNormalSequenceIndex = 0;

int ResetCount = 0;

int Activate = 0;

typedef struct user user;

char UnlockSequence[1] = {'#'};
char SectionA[1] = {'A'};
char SectionB[1] = {'B'};
char SectionC[1] = {'C'};


struct user{
	int userIndex;
	char code[32];
	char permissions[32];
	char sections[3];
};

int userAuthorized = 0;
user authorizedUser;

int SectionAArmed = 0;
int SectionBArmed = 0;
int SectionCArmed = 0;

int SectionAAlarm = 0;
int SectionBAlarm = 0;
int SectionCAlarm = 0;

user users[32];

#define Column_1_Pin GPIO_PIN_5
#define Column_1_Port GPIOB

#define Column_2_Pin GPIO_PIN_4
#define Column_2_Port GPIOB

#define Column_3_Pin GPIO_PIN_10
#define Column_3_Port GPIOB

#define Column_4_Pin GPIO_PIN_8
#define Column_4_Port GPIOA


#define Row_1_Pin GPIO_PIN_9
#define Row_1_Port GPIOA

#define Row_2_Pin GPIO_PIN_7
#define Row_2_Port GPIOC

#define Row_3_Pin GPIO_PIN_6
#define Row_3_Port GPIOB

#define Row_4_Pin GPIO_PIN_7
#define Row_4_Port GPIOA

#define SectionAIndicator_Pin 		GPIO_PIN_0
#define SectionAIndicator_Pin_Port  GPIOC
#define SectionBIndicator_Pin       GPIO_PIN_2
#define SectionBIndicator_Pin_Port  GPIOA
#define SectionCIndicator_Pin	    GPIO_PIN_3
#define SectionCIndicator_Pin_Port  GPIOB

#define AuthorizationIndicator_Pin	    GPIO_PIN_6
#define AuthorizationIndicator_Port	    GPIOA

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM21_Init(void);
static void MX_TIM22_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

void InitializeDisplaySPI(){

	//int n = sizeof(initialization_instructions) / sizeof(initialization_instructions[0]);

}

int CheckKey(GPIO_TypeDef* ColumnGPIOx, uint16_t ColumnGPIO_Pin,GPIO_TypeDef* RowGPIOx, uint16_t RowGPIO_Pin){
	if(HAL_GPIO_ReadPin(ColumnGPIOx, ColumnGPIO_Pin) && HAL_GPIO_ReadPin(RowGPIOx, RowGPIO_Pin)){
		HAL_Delay(50);
		if(HAL_GPIO_ReadPin(ColumnGPIOx, ColumnGPIO_Pin) && HAL_GPIO_ReadPin(RowGPIOx, RowGPIO_Pin)){
			return 1;
		}else{
			return 0;
		}
	}else{
		return 0;
	}
}

void Delay(uint32_t DelayTime){
	for (int i = 0; i < (DelayTime); i+=1){
		asm("nop");
	}
}

void PiezoPlus(uint32_t delay, uint32_t repeats){
	for (int i = 0; i < repeats;i++){
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 1);
		Delay(delay);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 0);
		Delay(delay);
	}
}

void Piezo(uint32_t delay){
	for (int i = 0; i < 100; i += 1){
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 1);
		Delay(delay);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 0);
		Delay(delay);
	}
}

void BeepError(){
	Piezo(3000);
	Piezo(3000);
}

void BeepSuccess(){

}

int AreStringsEqual(char* Arr1, char* Arr2){

	if(!((sizeof(Arr1)) == sizeof(Arr2))){
		return 0;
	}

	for(int i = 0; i < ((sizeof(Arr2) + 1)); i++){
		if(!(Arr1[i] == Arr2[i])){
			return 0;
		}
	}

	return 1;
}

void DetectCodeParameterless(char* Code, void (*f)()){
	if(AreStringsEqual(Code, CurrentNormalSequence)){
		memset(CurrentNormalSequence, 0, sizeof CurrentNormalSequence);
		CurrentNormalSequenceIndex = 0;
		(*f)();
	}
}

void Escape(){
	Piezo(3000);
	HAL_Delay(100);
	Piezo(2000);
	memset(CurrentNormalSequence, 0, sizeof CurrentNormalSequence);
	CurrentNormalSequenceIndex = 0;
}



char GetKey(){
	if(CheckKey(Column_1_Port, Column_1_Pin, Row_1_Port, Row_1_Pin)){
		Piezo(2000);
		return '1';
	}

	if(CheckKey(Column_2_Port, Column_2_Pin, Row_1_Port, Row_1_Pin)){
		Piezo(2000);
		return '2';
	}

	if(CheckKey(Column_3_Port, Column_3_Pin, Row_1_Port, Row_1_Pin)){
		Piezo(2000);
		return '3';
	}

//	if(CheckKey(Column_4_Port, Column_4_Pin, Row_1_Port, Row_1_Pin)){
//		Piezo(3000);
//		return 'A';
//	}

	if(CheckKey(Column_1_Port, Column_1_Pin, Row_2_Port, Row_2_Pin)){
		Piezo(2000);
		return '4';
	}

	if(CheckKey(Column_2_Port, Column_2_Pin, Row_2_Port, Row_2_Pin)){
		Piezo(2000);
		return '5';
	}

	if(CheckKey(Column_3_Port, Column_3_Pin, Row_2_Port, Row_2_Pin)){
		Piezo(2000);
		return '6';
	}

//	if(CheckKey(Column_4_Port, Column_4_Pin, Row_2_Port, Row_2_Pin)){
//		Piezo(3000);
//		return 'B';
//	}


	if(CheckKey(Column_1_Port, Column_1_Pin, Row_3_Port, Row_3_Pin)){
		Piezo(2000);
		return '7';
	}

	if(CheckKey(Column_2_Port, Column_2_Pin, Row_3_Port, Row_3_Pin)){
		Piezo(2000);
		return '8';
	}

	if(CheckKey(Column_3_Port, Column_3_Pin, Row_3_Port, Row_3_Pin)){
		Piezo(2000);
		return '9';
	}

//	if(CheckKey(Column_4_Port, Column_4_Pin, Row_3_Port, Row_3_Pin)){
//		Piezo(3000);
//		return 'C';
//	}

	if(CheckKey(Column_1_Port, Column_1_Pin, Row_4_Port, Row_4_Pin)){
		Piezo(3000);
		return '*';
	}

	if(CheckKey(Column_2_Port, Column_2_Pin, Row_4_Port, Row_4_Pin)){
		Piezo(2000);
		return '0';
	}

	if(CheckKey(Column_3_Port, Column_3_Pin, Row_4_Port, Row_4_Pin)){
		Piezo(3000);
		return '#';
	}

	if(CheckKey(Column_4_Port, Column_4_Pin, Row_4_Port, Row_4_Pin)){
		Piezo(3000);
		return 'D';
	}
	return 'X';
}

char ReturnKey(){
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 1);
	char currentKey = GetKey();
	if (currentKey != 'X'){
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 0);
		return currentKey;
	}
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 0);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 1);
	currentKey = GetKey();
	if (currentKey != 'X'){
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 0);
		return currentKey;

	}
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 0);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, 1);
	currentKey = GetKey();
	if (currentKey != 'X'){
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, 0);
		return currentKey;

	}
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, 0);

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 1);
	currentKey = GetKey();
	if (currentKey != 'X'){
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 0);
		return currentKey;

	}
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 0);

	return 'X';
}

void Authorize(user userToAuthorize){
	Piezo(3000);
	Piezo(2000);
	Piezo(1000);

	HAL_GPIO_WritePin(AuthorizationIndicator_Port, AuthorizationIndicator_Pin, 1);

	userAuthorized = 1;
	authorizedUser = userToAuthorize;

	if(SectionAAlarm && strpbrk("A", authorizedUser.sections) != 0) SectionAAlarm = 0;
	if(SectionBAlarm && strpbrk("B", authorizedUser.sections) != 0) SectionBAlarm = 0;
	if(SectionCAlarm && strpbrk("C", authorizedUser.sections) != 0) SectionCAlarm = 0;
}

void Authorization(){
	int userSelected = 0;
	char InputSequence[32];
	int InputSequenceIndex = 0;

	char UserSelect[3] = {0, 0, 0};
	int UserSelectIndex = 0;

	user selectedUser;
	HAL_TIM_Base_Stop_IT(&htim6);
	while(1){
		while(!userSelected){
			char PressedKey = ReturnKey();

			if(PressedKey == 'D'){
				HAL_Delay(300);
				if(HAL_GPIO_ReadPin(Row_4_Port, Row_4_Pin)){
					Piezo(1000);
					return;
				}
			}

			if(PressedKey == '1' || PressedKey == '2' || PressedKey == '3' || PressedKey == '4' || PressedKey == '5' || PressedKey == '6' || PressedKey == '7' ||
					PressedKey == '8' || PressedKey == '9' || PressedKey == '0'){
				UserSelect[UserSelectIndex] = PressedKey;
				UserSelectIndex += 1;
			}

			int EnteredInt = atoi(UserSelect);

			if(PressedKey == '*'){
				int Failed = 1;
				for(int i = 0; i < 33 ; i += 1){
					if(i >= 33){
						break;
					}
					if(users[i].userIndex == EnteredInt){
						selectedUser = users[i];
						userSelected = 1;
						Failed = 0;
						break;
					}
				}
				if(Failed){
					BeepError();
					return;
				}
			}
		}

		Piezo(1500);
		HAL_Delay(500);
		Piezo(1500);

		if(userSelected){
			int indBlink = 0;
			while(1){

				if(indBlink == 0){
					HAL_GPIO_WritePin(AuthorizationIndicator_Port, AuthorizationIndicator_Pin, 1);
				}
				indBlink += 1;

				char inputKey = ReturnKey();

				if(inputKey == 'D'){
					HAL_Delay(300);
					if(HAL_GPIO_ReadPin(Row_4_Port, Row_4_Pin)){
						Piezo(1000);
						return;
					}
				}

				if(inputKey == '1' || inputKey == '2' || inputKey == '3' || inputKey == '4' || inputKey == '5' || inputKey == '6' || inputKey == '7' ||
						inputKey == '8' || inputKey == '9' || inputKey == '0'){
					InputSequence[InputSequenceIndex] = inputKey;
					InputSequenceIndex += 1;
				}

				if(inputKey == '*'){
					if (AreStringsEqual(selectedUser.code, InputSequence)){
						Authorize(selectedUser);
						return;
					}else{
						BeepError();
						return;
					}
				}

				if(indBlink == 10000){
					HAL_GPIO_WritePin(AuthorizationIndicator_Port, AuthorizationIndicator_Pin, 0);
				}
				if(indBlink == 20000){
					indBlink = 0;
				}
			}
		}
	}
}

void DisarmSection(char sectionToDisarm){
	if(sectionToDisarm == 'A'){
		SectionAArmed = 0;
		Piezo(3000);
		Piezo(2000);
	}
	if(sectionToDisarm == 'B'){
		SectionBArmed = 0;
		Piezo(3000);
		Piezo(2000);
	}
	if(sectionToDisarm == 'C'){
		SectionCArmed = 0;
		Piezo(3000);
		Piezo(2000);
	}
}

void ArmSection(char sectionToArm){
	if(sectionToArm == 'A'){
		SectionAArmed = 1;
		Piezo(2000);
		Piezo(3000);
	}
	if(sectionToArm == 'B'){
		SectionBArmed = 1;
		Piezo(2000);
		Piezo(3000);
	}
	if(sectionToArm == 'C'){
		SectionCArmed = 1;
		Piezo(2000);
		Piezo(3000);
	}
}

void ControlSection(char param){
	if(userAuthorized){
		if ((strpbrk("A", authorizedUser.sections) != 0) && (param == 'A')){
			if(SectionAArmed){
				DisarmSection('A');
			}else{
				ArmSection('A');
			}
		}
		if ((strpbrk("B", authorizedUser.sections) != 0) && (param == 'B')){
			if(SectionBArmed){
				DisarmSection('B');
			}else{
				ArmSection('B');
			}
		}
		if ((strpbrk("C", authorizedUser.sections) != 0) && (param == 'C')){
			if(SectionCArmed){
				DisarmSection('C');
			}else{
				ArmSection('C');
			}
		}
	}else{
		BeepError();
	}
}

void CheatCodeFunc(){
	Authorize(users[0]);
}

void KeyRegistered(char key){
	HAL_TIM_Base_Start_IT(&htim6);

	if(key == 'D'){
		HAL_Delay(300);
		if(HAL_GPIO_ReadPin(Row_4_Port, Row_4_Pin)){
			Piezo(1000);
			Escape();
			return;
		}
	}

	if(key == 'A'){
		ControlSection('A');
		return;
	}
	if(key == 'B'){
		ControlSection('B');
		return;
	}
	if(key == 'C'){
		ControlSection('C');
		return;
	}

	TIM6->CNT = 0;
	ResetCount = 1;

	CurrentNormalSequence[CurrentNormalSequenceIndex] = key;
	CurrentNormalSequenceIndex += 1;

	char CheatCode[5] = {'1', '2', '3', '4', '5'};

	DetectCodeParameterless(UnlockSequence, &Authorization);
	DetectCodeParameterless(CheatCode, &CheatCodeFunc);

	if(!Activate){
		Activate = 1;
	}
}

void KeyPad_Check(){
	if(CheckKey(Column_1_Port, Column_1_Pin, Row_1_Port, Row_1_Pin)){
		Piezo(2000);
		KeyRegistered('1');
		return;
	}

	if(CheckKey(Column_2_Port, Column_2_Pin, Row_1_Port, Row_1_Pin)){
		Piezo(2000);
		KeyRegistered('2');
		return;
	}

	if(CheckKey(Column_3_Port, Column_3_Pin, Row_1_Port, Row_1_Pin)){
		Piezo(2000);
		KeyRegistered('3');
		return;
	}

	if(CheckKey(Column_4_Port, Column_4_Pin, Row_1_Port, Row_1_Pin)){
		Piezo(3000);
		KeyRegistered('A');
		return;
	}

	if(CheckKey(Column_1_Port, Column_1_Pin, Row_2_Port, Row_2_Pin)){
		Piezo(2000);
		KeyRegistered('4');
		return;
	}

	if(CheckKey(Column_2_Port, Column_2_Pin, Row_2_Port, Row_2_Pin)){
		Piezo(2000);
		KeyRegistered('5');
		return;
	}

	if(CheckKey(Column_3_Port, Column_3_Pin, Row_2_Port, Row_2_Pin)){
		Piezo(2000);
		KeyRegistered('6');
		return;
	}

	if(CheckKey(Column_4_Port, Column_4_Pin, Row_2_Port, Row_2_Pin)){
		Piezo(3000);
		KeyRegistered('B');
		return;
	}


	if(CheckKey(Column_1_Port, Column_1_Pin, Row_3_Port, Row_3_Pin)){
		Piezo(2000);
		KeyRegistered('7');
		return;
	}

	if(CheckKey(Column_2_Port, Column_2_Pin, Row_3_Port, Row_3_Pin)){
		Piezo(2000);
		KeyRegistered('8');
		return;
	}

	if(CheckKey(Column_3_Port, Column_3_Pin, Row_3_Port, Row_3_Pin)){
		Piezo(2000);
		KeyRegistered('9');
		return;
	}

	if(CheckKey(Column_4_Port, Column_4_Pin, Row_3_Port, Row_3_Pin)){
		Piezo(3000);
		KeyRegistered('C');
		return;
	}

	if(CheckKey(Column_1_Port, Column_1_Pin, Row_4_Port, Row_4_Pin)){
		Piezo(3000);
		KeyRegistered('*');
		return;
	}

	if(CheckKey(Column_2_Port, Column_2_Pin, Row_4_Port, Row_4_Pin)){
		Piezo(2000);
		KeyRegistered('0');
		return;
	}

	if(CheckKey(Column_3_Port, Column_3_Pin, Row_4_Port, Row_4_Pin)){
		Piezo(3000);
		KeyRegistered('#');
		return;
	}

	if(CheckKey(Column_4_Port, Column_4_Pin, Row_4_Port, Row_4_Pin)){
		Piezo(3000);
		KeyRegistered('D');
		return;
	}
}

void KeyPad(){
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 1);
	KeyPad_Check();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 0);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 1);
	KeyPad_Check();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 0);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, 1);
	KeyPad_Check();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, 0);

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 1);
	KeyPad_Check();
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 0);
}

void Indicators(){
	if(!SectionAAlarm && !SectionBAlarm && !SectionCAlarm){
		HAL_GPIO_WritePin(SectionAIndicator_Pin_Port, SectionAIndicator_Pin, !SectionAArmed);
		HAL_GPIO_WritePin(SectionBIndicator_Pin_Port, SectionBIndicator_Pin, !SectionBArmed);
		HAL_GPIO_WritePin(SectionCIndicator_Pin_Port, SectionCIndicator_Pin, !SectionCArmed);
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(Activate){
		if(ResetCount <= 1){
			Piezo(500);
			memset(CurrentNormalSequence, 0, sizeof CurrentNormalSequence);
			CurrentNormalSequenceIndex = 0;
			ResetCount += 1;
			userAuthorized = 0;
			HAL_GPIO_WritePin(AuthorizationIndicator_Port, AuthorizationIndicator_Pin, 0);
		}
	}
}

void IntializeSystemUsers(){
	  user ROOT;
	  ROOT.userIndex = 0;
	  ROOT.code[0] = '4';
	  ROOT.code[1] = '5';
	  ROOT.code[2] = '3';
	  ROOT.code[3] = '5';
	  ROOT.code[4] = '4';
	  ROOT.code[5] = '1';
	  ROOT.code[6] = '2';
	  ROOT.code[7] = '4';
	  ROOT.code[8] = '\0';
	  ROOT.sections[0] = 'A';
	  ROOT.sections[1] = 'B';
	  ROOT.sections[2] = 'C';
	  ROOT.permissions[0] = 'R';

	  users[0] = ROOT;

	  user SectionAUser;
	  SectionAUser.userIndex = 1;
	  SectionAUser.code[0] = '4';
	  SectionAUser.code[1] = '5';
	  SectionAUser.code[2] = '3';
	  SectionAUser.code[3] = '2';
	  SectionAUser.code[4] = '8';
	  SectionAUser.sections[0] = 'A';

	  users[1] = SectionAUser;
}

void AlarmBlink(){
	HAL_GPIO_WritePin(SectionAIndicator_Pin_Port, SectionAIndicator_Pin, 1);
	HAL_Delay(100);
	HAL_GPIO_WritePin(SectionAIndicator_Pin_Port, SectionAIndicator_Pin, 0);
	HAL_Delay(100);
}

void Alarms(){
	if((SectionAArmed && !HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13)) || SectionAAlarm){
		AlarmBlink();
		if(!SectionAAlarm) SectionAAlarm = 1;
	}
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM6_Init();
  MX_TIM21_Init();
  MX_TIM22_Init();
  /* USER CODE BEGIN 2 */

  IntializeSystemUsers();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  for (;;){
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
	  KeyPad();
	  Alarms();
	  Indicators();
  }
  /* USER CODE END 3 */

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Configure the main internal regulator output voltage 
    */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* TIM6 init function */
static void MX_TIM6_Init(void)
{

  TIM_MasterConfigTypeDef sMasterConfig;

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 5000;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 32768;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM21 init function */
static void MX_TIM21_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_SlaveConfigTypeDef sSlaveConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim21.Instance = TIM21;
  htim21.Init.Prescaler = 1000;
  htim21.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim21.Init.Period = 32768;
  htim21.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim21) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim21, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_RESET;
  sSlaveConfig.InputTrigger = TIM_TS_ITR0;
  if (HAL_TIM_SlaveConfigSynchronization(&htim21, &sSlaveConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim21, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM22 init function */
static void MX_TIM22_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_SlaveConfigTypeDef sSlaveConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim22.Instance = TIM22;
  htim22.Init.Prescaler = 1000;
  htim22.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim22.Init.Period = 32768;
  htim22.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim22) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim22, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_RESET;
  sSlaveConfig.InputTrigger = TIM_TS_ITR1;
  if (HAL_TIM_SlaveConfigSynchronization(&htim22, &sSlaveConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim22, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Section_GPIO_Port, Section_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SectionA2_Pin|SectionA5_Pin|AuthorizationIndicator_Pin|KeypadA8_Pin 
                          |GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, KeypadB10_Pin|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pins : User_button_Pin KeypadC7_Pin */
  GPIO_InitStruct.Pin = User_button_Pin|KeypadC7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : Section_Pin */
  GPIO_InitStruct.Pin = Section_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(Section_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SectionA2_Pin SectionA5_Pin PA10 */
  GPIO_InitStruct.Pin = SectionA2_Pin|SectionA5_Pin|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : AuthorizationIndicator_Pin KeypadA8_Pin */
  GPIO_InitStruct.Pin = AuthorizationIndicator_Pin|KeypadA8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : Keypad_Pin PA9 */
  GPIO_InitStruct.Pin = Keypad_Pin|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : KeypadB10_Pin PB3 PB4 PB5 */
  GPIO_InitStruct.Pin = KeypadB10_Pin|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : KeypadB6_Pin */
  GPIO_InitStruct.Pin = KeypadB6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(KeypadB6_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

#define CARRIORCNT 2400 - 1 // クロック周波数48000000 2400で山谷キャリア10kHz
#define Ts 100E-6f
#define DEADTIMECNT 48 // 1us
#define interruptPin 2

#define TWOPI           6.283185307f
#define SQRT_2DIV3			0.816496581f
#define SQRT3_DIV3			0.86602540378f

#include <math.h>

float theta = 0.0f;

void setup() {
  pinMode(4, OUTPUT);       // D4ピンを出力に設定（処理負荷計測用）
  pinMode(interruptPin, INPUT_PULLUP); // D2ピンを入力に設定（山割り込み用）
  
  // 割込み関数を設定
  attachInterrupt(digitalPinToInterrupt(interruptPin), interruptDo, RISING);

  // GTP16用 モジュールストップ状態を解除
  R_MSTP->MSTPCRD_b.MSTPD6 = 0; 

  // PWM出力用端子の設定
  // ピンを出力に設定
  R_PFS->PORT[1].PIN[2].PmnPFS_b.PDR = 1;
  R_PFS->PORT[1].PIN[11].PmnPFS_b.PDR = 1;
  R_PFS->PORT[1].PIN[12].PmnPFS_b.PDR = 1;
  R_PFS->PORT[4].PIN[11].PmnPFS_b.PDR = 1;
  R_PFS->PORT[4].PIN[10].PmnPFS_b.PDR = 1; 
  R_PFS->PORT[3].PIN[4].PmnPFS_b.PDR = 1;
  R_PFS->PORT[3].PIN[3].PmnPFS_b.PDR = 1; 
  
  //周辺選択ビットでGPTを設定
  R_PFS->PORT[1].PIN[2].PmnPFS_b.PSEL = 0b00011;
  R_PFS->PORT[1].PIN[11].PmnPFS_b.PSEL = 0b00011;
  R_PFS->PORT[1].PIN[12].PmnPFS_b.PSEL = 0b00011;
  R_PFS->PORT[4].PIN[11].PmnPFS_b.PSEL = 0b00011;
  R_PFS->PORT[4].PIN[10].PmnPFS_b.PSEL = 0b00011;
  R_PFS->PORT[3].PIN[4].PmnPFS_b.PSEL = 0b00011;
  R_PFS->PORT[3].PIN[3].PmnPFS_b.PSEL = 0b00011;

  // ポートモード制御ビットで周辺機能に設定
  R_PFS->PORT[1].PIN[2].PmnPFS_b.PMR = 1; 
  R_PFS->PORT[1].PIN[11].PmnPFS_b.PMR = 1;
  R_PFS->PORT[1].PIN[12].PmnPFS_b.PMR = 1;
  R_PFS->PORT[4].PIN[11].PmnPFS_b.PMR = 1;
  R_PFS->PORT[4].PIN[10].PmnPFS_b.PMR = 1;
  R_PFS->PORT[3].PIN[4].PmnPFS_b.PMR = 1;
  R_PFS->PORT[3].PIN[3].PmnPFS_b.PMR = 1;
  
  // PWMタイマの同期
  // GTSTR、GTSTPによるカウンタスタート／ストップを許可
  R_GPT2->GTSSR_b.CSTRT = 1;
  R_GPT3->GTSSR_b.CSTRT = 1;
  R_GPT6->GTSSR_b.CSTRT = 1;
  R_GPT7->GTSSR_b.CSTRT = 1;

  //GTSTPによるカウンタストップ（全てのタイマでレジスタは共通）
  R_GPT2->GTSTP = 0b11001100; 
 
  // 搬送波を三角波に設定（三角波PWMモード1、バッファ転送は谷のみ。山谷の場合はPWMモード2を選択）
  R_GPT2->GTCR_b.MD = 0b100;
  R_GPT3->GTCR_b.MD = 0b100;
  R_GPT6->GTCR_b.MD = 0b100;
  R_GPT7->GTCR_b.MD = 0b100;
  
  // GTIOCA, GTIOCB端子の出力許可
  R_GPT2->GTIOR_b.OBE = 1; 
  R_GPT3->GTIOR_b.OAE = 1;
  R_GPT3->GTIOR_b.OBE = 1; 
  R_GPT6->GTIOR_b.OAE = 1;
  R_GPT6->GTIOR_b.OBE = 1; 
  R_GPT7->GTIOR_b.OAE = 1;
  R_GPT7->GTIOR_b.OBE = 1;

  // Dutyのバッファ設定
  // 01bでシングルバッファ
  R_GPT2->GTBER_b.CCRB = 0b01;
  R_GPT3->GTBER_b.CCRA = 0b01;
  R_GPT3->GTBER_b.CCRB = 0b01;
  R_GPT6->GTBER_b.CCRA = 0b01;
  R_GPT6->GTBER_b.CCRB = 0b01;
  R_GPT7->GTBER_b.CCRA = 0b01;
  R_GPT7->GTBER_b.CCRB = 0b01; 

  // PWMのHigh,Row設定 
  // GPT2はBchからGPT3、6、7の山側でトリガ信号を発生させるための設定が必要
  R_GPT2->GTIOR_b.GTIOB = 0b10011;
  // GPT3、6、7は相補PWM実現のためAchとBchで別の設定が必要
  R_GPT3->GTIOR_b.GTIOA = 0b00011;
  R_GPT3->GTIOR_b.GTIOB = 0b10011;
  R_GPT6->GTIOR_b.GTIOA = 0b00011;
  R_GPT6->GTIOR_b.GTIOB = 0b10011;
  R_GPT7->GTIOR_b.GTIOA = 0b00011;
  R_GPT7->GTIOR_b.GTIOB = 0b10011;

  // キャリア周波数設定
  R_GPT2->GTPR = CARRIORCNT;
  R_GPT3->GTPR = CARRIORCNT;
  R_GPT6->GTPR = CARRIORCNT;
  R_GPT7->GTPR = CARRIORCNT;

  // デッドタイム自動設定
  R_GPT3->GTDTCR_b.TDE = 1;
  R_GPT6->GTDTCR_b.TDE = 1;
  R_GPT7->GTDTCR_b.TDE = 1;

  //デッドタイム設定
  R_GPT6->GTDVU = DEADTIMECNT;
  R_GPT7->GTDVU = DEADTIMECNT; 
  R_GPT3->GTDVU = DEADTIMECNT;
  
  // Duty設定
  R_GPT2->GTCCR[3] = 10;    // GPT2はBchを使用、かつバッファ設定のため[3]にDutyを設定
  R_GPT6->GTCCR[2] = CARRIORCNT - 1000;  // バッファ設定のため[2]にDutyを設定 山谷を逆として扱うため引き算にする
  R_GPT7->GTCCR[2] = CARRIORCNT - 1000;  
  R_GPT3->GTCCR[2] = CARRIORCNT - 1000;  


  // カウンタリセット
  R_GPT2->GTCNT = 0; 
  R_GPT3->GTCNT = 0;
  R_GPT6->GTCNT = 0;
  R_GPT7->GTCNT = 0; 

  //GTSTRによるカウンタスタート（全てのタイマでレジスタは共通）
  R_GPT2->GTSTR = 0b11001100; 

}

void interruptDo(){
  float omega_ref = 400.0f;
  float sinTheta;
  float cosTheta;
  float dq[2];
  float ab[2];
  float uvw[3];
  float Duty[3];
  uint16_t CNT[3];
  float twoDivVdc = 0.2f; // Vdc = 10V

  R_PORT1->PODR_b.PODR6 = 1;

  //Calc Theta
  theta = theta + omega_ref * Ts;
  if(theta > TWOPI) theta = 0.0f;
  sinTheta = sinf(theta); 
  cosTheta = cosf(theta);
  

  // Calc Voltage
  dq[0] = 0.0f;
  dq[1] = 0.5f;

  ab[0] = dq[0] * cosTheta - dq[1] * sinTheta;
  ab[1] = dq[0] * sinTheta + dq[1] * cosTheta;
  uvw[0] = SQRT_2DIV3 * ab[0];
  uvw[1] = SQRT_2DIV3 * ( -0.5f * ab[0] + SQRT3_DIV3 * ab[1] );
  uvw[2] = - uvw[0] - uvw[1];

  // 50% Center
  Duty[0] = uvw[0] * twoDivVdc + 0.5f;
  Duty[1] = uvw[1] * twoDivVdc + 0.5f;
  Duty[2] = uvw[2] * twoDivVdc + 0.5f;

  CNT[0] = uint16_t( Duty[0] * (float)CARRIORCNT );
  CNT[1] = uint16_t( Duty[1] * (float)CARRIORCNT );
  CNT[2] = uint16_t( Duty[2] * (float)CARRIORCNT );

  // 反転処理
  R_GPT3->GTCCR[2] = CARRIORCNT - CNT[0];
  R_GPT7->GTCCR[2] = CARRIORCNT - CNT[1];
  R_GPT6->GTCCR[2] = CARRIORCNT - CNT[2];

  R_PORT1->PODR_b.PODR6 = 0;
  
}

void loop() {
}


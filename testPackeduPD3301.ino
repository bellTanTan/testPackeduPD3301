#include "Arduino.h"

#include "fabgl.h"
#include "fabutils.h"

#include "packeduPD3301adpter.h"

#define SD_MOUNT_PATH       "/SD"
#define PC8001FONT          "/PC8001/PC-8001.FON"
#define ARRAY_SIZE( array ) ( (int)( sizeof( array ) / sizeof( (array)[0] ) ) )
#define HLT                 { while ( 1 ) { delay( 500 ); } }

#define VIDEO_MEM_SIZE      (80 * 2 * 25)

packeduPD3301adpter m_packeduPD3301;
static  uint8_t * s_videoMemory;
uint8_t * m_frameBuffer;
uint8_t * m_FONT;
FontInfo  m_FontPC8001;
FontInfo  m_FontPC8001Graph;
FontInfo  m_FontPCG;

time_t t0;
int tx;
int ty;
int seqNo;
bool fEnable;


void makeGraphFont( void )
{
  for ( int i = 0; i < 16; i++ )
  {
    uint64_t * p = (uint64_t *)&m_FONT[2048+i*8*16];
    uint64_t bitData1 = 0x0F0F;
    uint64_t bitData2 = 0xF0F0;
    uint64_t bitData3 = 0;
    if ( i & 0x01 )
      bitData3 |= bitData1;
    if ( i & 0x02 )
      bitData3 |= ( bitData1 << 16 );
    if ( i & 0x04 )
      bitData3 |= ( bitData1 << 32 );
    if ( i & 0x08 )
      bitData3 |= ( bitData1 << 48 );
    for ( int j = 0; j < 16; j++ )
    {
      uint64_t setData = bitData3;
      if ( j & 0x01 )
        setData |= bitData2;
      if ( j & 0x02 )
        setData |= ( bitData2 << 16 );
      if ( j & 0x04 )
        setData |= ( bitData2 << 32 );
      if ( j & 0x08 )
        setData |= ( bitData2 << 48 );
      p[j] = setData;  
    }
  }
}


bool fontload( const char * fileName )
{
  size_t fontSize = 256 * 8 * 3;
  m_FONT = (uint8_t *)ps_malloc( fontSize );
  if ( !m_FONT ) return false;
  memset( m_FONT, 0, fontSize ); 
  char path[256];
  strcpy( path, SD_MOUNT_PATH );
  strcat( path, fileName );
  bool fontload = false;
  auto fp = fopen( path, "r" );
  if ( !fp ) return false;
  fseek( fp, 0, SEEK_END );
  size_t fileSize = ftell( fp );
  fseek( fp, 0, SEEK_SET );
  if ( fileSize == 2048 || fileSize == 4096 )
  {
    fread( &m_FONT[0], 1, fileSize, fp );
    fontload = true;
  }
  fclose( fp );
  if ( !fontload ) return false;
  if ( fileSize == 2048 )
    makeGraphFont();

  m_FontPC8001.pointSize = 6;
  m_FontPC8001.width     = 8;
  m_FontPC8001.height    = 8;
  m_FontPC8001.ascent    = 7;
  m_FontPC8001.inleading = 0;
  m_FontPC8001.exleading = 0;
  m_FontPC8001.flags     = 0;
  m_FontPC8001.weight    = 400;
  m_FontPC8001.charset   = 255;
  m_FontPC8001.data      = &m_FONT[0];
  m_FontPC8001.chptr     = NULL;
  m_FontPC8001.codepage  = 437;

  m_FontPC8001Graph.pointSize = 6;
  m_FontPC8001Graph.width     = 8;
  m_FontPC8001Graph.height    = 8;
  m_FontPC8001Graph.ascent    = 7;
  m_FontPC8001Graph.inleading = 0;
  m_FontPC8001Graph.exleading = 0;
  m_FontPC8001Graph.flags     = 0;
  m_FontPC8001Graph.weight    = 400;
  m_FontPC8001Graph.charset   = 255;
  m_FontPC8001Graph.data      = &m_FONT[2048];
  m_FontPC8001Graph.chptr     = NULL;
  m_FontPC8001Graph.codepage  = 437;

  m_FontPCG.pointSize = 6;
  m_FontPCG.width     = 8;
  m_FontPCG.height    = 8;
  m_FontPCG.ascent    = 7;
  m_FontPCG.inleading = 0;
  m_FontPCG.exleading = 0;
  m_FontPCG.flags     = 0;
  m_FontPCG.weight    = 400;
  m_FontPCG.charset   = 255;
  m_FontPCG.data      = &m_FONT[4096];
  m_FontPCG.chptr     = NULL;
  m_FontPCG.codepage  = 437;
  return true;
}


void setup()
{
  Serial.begin( 115200 );
  while ( !Serial && !Serial.available() );
  Serial.println();

  if ( !FileBrowser::mountSDCard( false, SD_MOUNT_PATH ) )
  {
    Serial.printf( "mountSDCard() failed." );
    Serial.println();
    HLT;
  }

  if ( !fontload( PC8001FONT ) )
  {
    Serial.printf( "fontload() failed." );
    Serial.println();
    HLT;
  }

  s_videoMemory = (uint8_t*)heap_caps_malloc( VIDEO_MEM_SIZE, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL );
  if ( s_videoMemory == NULL )
  {
    Serial.printf( "heap_caps_malloc() failed." );
    Serial.println();
    HLT;
  }
  m_frameBuffer = s_videoMemory;
  m_packeduPD3301.setVideoBuffer( m_frameBuffer );
  m_packeduPD3301.setFont( &m_FontPC8001, &m_FontPC8001Graph, &m_FontPCG );
  m_packeduPD3301.setEmulation( packeduPD3301adpter::Emulation::PC_Text_40x20_8Colors );
  //m_packeduPD3301.setEmulation( packeduPD3301adpter::Emulation::PC_Text_40x25_8Colors );
  //m_packeduPD3301.setEmulation( packeduPD3301adpter::Emulation::PC_Text_80x20_8Colors );
  //m_packeduPD3301.setEmulation( packeduPD3301adpter::Emulation::PC_Text_80x25_8Colors );
  m_packeduPD3301.enableVideo( true );

  for ( int i = 0; i < VIDEO_MEM_SIZE; i++ )
    m_frameBuffer[i] = 0;

  m_frameBuffer[ 0] = 'N';
  m_frameBuffer[ 1] = 0b00100111;
  m_frameBuffer[ 2] = 'E';
  m_frameBuffer[ 3] = 0b10000110;
  m_frameBuffer[ 4] = 'C';
  m_frameBuffer[ 5] = 0b00100101;
  m_frameBuffer[ 6] = ' ';
  m_frameBuffer[ 7] = 0b00000111;
  m_frameBuffer[ 8] = 'P';
  m_frameBuffer[ 9] = 0b00100100;
  m_frameBuffer[10] = 'C';
  m_frameBuffer[11] = 0b00000011;
  m_frameBuffer[12] = '-';
  m_frameBuffer[13] = 0b00100010;
  m_frameBuffer[14] = '8';
  m_frameBuffer[15] = 0b00000001;
  m_frameBuffer[16] = '0';
  m_frameBuffer[17] = 0b00100111;
  m_frameBuffer[18] = '0';
  m_frameBuffer[19] = 0b00000111;
  m_frameBuffer[20] = '1';
  m_frameBuffer[21] = 0b00000111;
  m_frameBuffer[22] = ' ';
  m_frameBuffer[23] = 0b00000111;
  m_frameBuffer[24] = 'A';
  m_frameBuffer[25] = 0b00010001;
  m_frameBuffer[26] = 'B';
  m_frameBuffer[27] = 0b00010010;
  m_frameBuffer[28] = 'C';
  m_frameBuffer[29] = 0b00010011;
  m_frameBuffer[30] = 'D';
  m_frameBuffer[31] = 0b00010100;
  m_frameBuffer[32] = 'E';
  m_frameBuffer[33] = 0b00010101;
  m_frameBuffer[34] = 'F';
  m_frameBuffer[35] = 0b00010110;
  m_frameBuffer[36] = 'G';
  m_frameBuffer[37] = 0b00010111;
  m_frameBuffer[38] = 'A';
  m_frameBuffer[39] = 0b01100001;
  m_frameBuffer[40] = 'B';
  m_frameBuffer[41] = 0b01000010;
  m_frameBuffer[42] = 'C';
  m_frameBuffer[43] = 0b00001011;
  m_frameBuffer[44] = 'D';
  m_frameBuffer[45] = 0b00001100;
  m_frameBuffer[46] = 'E';
  m_frameBuffer[47] = 0b10000101;
  m_frameBuffer[48] = 'F';
  m_frameBuffer[49] = 0b10000110;
  m_frameBuffer[50] = 'G';
  m_frameBuffer[51] = 0b10000111;

  for ( int i = 1; i < 25; i++ )
  {
    char buf[128];
    sprintf( buf, "%02d Sample Data", i );
    int adrs = ( 40 * 2 ) * i + ( 2 * 0 );
    uint8_t color = 0;
    int len = strlen( buf );
    for ( int j = 0; j < len; j++ )
    {
      if ( j >= 0 && j < 5 )
        color = 0b10000111;
      else
        color = 0b00001111;
      m_frameBuffer[adrs+0] = buf[j];
      m_frameBuffer[adrs+1] = color;
      adrs += 2;
    }
  }

  m_packeduPD3301.setCursorPos( 0, 0 );
  m_packeduPD3301.setCursorVisible( true );

  time( &t0 );
  tx = 0;
  ty = 0;
  fEnable = false;
  seqNo = 0;
}

void loop()
{
#if 0
  time_t t1;
  time( &t1 );

  if ( ( t1 - t0 ) >= 5 )
  {
    t0 = t1;
    tx++;
    if ( tx > 24 ) tx = 0;
    m_packeduPD3301.setCursorPos( ty, tx );
    m_packeduPD3301.enablePCG( fEnable );
    if ( fEnable )
      fEnable = false;
    else
      fEnable = true;
  }
#endif  

#if 1
  time_t t1;
  time( &t1 );

  if ( ( t1 - t0 ) >= 5 )
  {
    t0 = t1;
    packeduPD3301adpter::Emulation disptype = packeduPD3301adpter::Emulation::None;
    switch ( seqNo )
    {
      case 0:
        disptype = packeduPD3301adpter::Emulation::PC_Text_40x20_8Colors;
        seqNo = 10;
        break;
      case 10:
        disptype = packeduPD3301adpter::Emulation::PC_Text_40x25_8Colors;
        seqNo = 20;
        break;
      case 20:
        disptype = packeduPD3301adpter::Emulation::PC_Text_80x20_8Colors;
        seqNo = 30;
        break;
      case 30:
        disptype = packeduPD3301adpter::Emulation::PC_Text_80x25_8Colors;
        seqNo = 0;
        break;
    }
    m_packeduPD3301.setEmulation( disptype );
  }
#endif  
}

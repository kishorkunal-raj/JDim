// ライセンス: GPL2

//#define _DEBUG
//#define _DEBUG_ICONV
#include "jddebug.h"

#include "jdiconv.h"
#include "miscmsg.h"
#include "miscutil.h"

#include <errno.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>


using namespace JDLIB;

Iconv::Iconv( const std::string& coding_to, const std::string& coding_from )
    : m_coding_from( coding_from )
{
#ifdef _DEBUG
    std::cout << "Iconv::Iconv coding = " << m_coding_from << " to " << coding_to << std::endl;
#endif
    
    m_buf_in = ( char* )malloc( BUF_SIZE_ICONV_IN );
    m_buf_out = ( char* )malloc( BUF_SIZE_ICONV_OUT );
    
    m_cd = g_iconv_open( coding_to.c_str(), m_coding_from.c_str() );

    // MS932で失敗したらCP932で試してみる
    if( m_cd == ( GIConv ) -1 ){
        if( coding_to == "MS932" ) m_cd = g_iconv_open( "CP932", m_coding_from.c_str() );
        else if( coding_from == "MS932" ) m_cd = g_iconv_open( coding_to.c_str(), "CP932" );
    }

    // "EUCJP-*"で失敗したら"EUCJP"で試してみる
    if( m_cd == ( GIConv ) - 1 && ( errno & EINVAL ) != 0 )
    {
        if( coding_to.find( "EUCJP-", 0 ) == 0 )
		{
            m_cd = g_iconv_open( "EUCJP//TRANSLIT", coding_from.c_str() );
        }
        else if( coding_from.find( "EUCJP-", 0 ) == 0 )
		{
            const std::string coding_to_translit = coding_to + "//TRANSLIT";
            m_cd = g_iconv_open( coding_to_translit.c_str(), "EUCJP" );
        }
    }

    if( m_cd == ( GIConv ) -1 ){
        MISC::ERRMSG( "can't open iconv coding = " + m_coding_from + " to " + coding_to );
    }
}

Iconv::~Iconv()
{
#ifdef _DEBUG
    std::cout << "Iconv::~Iconv\n";
#endif    
    
    if( m_buf_in ) free( m_buf_in );
    if( m_buf_out ) free( m_buf_out );
    if( m_cd != ( GIConv ) -1 ) g_iconv_close( m_cd );
}


const char* Iconv::convert( char* str_in, int size_in, int& size_out )
{
#ifdef _DEBUG
    std::cout << "Iconv::convert size_in = " << size_in 
              <<" left = " << m_byte_left_in << std::endl;
#endif

    assert( m_byte_left_in + size_in < BUF_SIZE_ICONV_IN );
    if( m_cd == ( GIConv ) -1 ) return nullptr;
    
    size_t byte_left_out = BUF_SIZE_ICONV_OUT;
    char* buf_out = m_buf_out;

    // 前回の残りをコピー
    if( m_byte_left_in ){
        memcpy( m_buf_in, m_buf_in_tmp, m_byte_left_in );
        m_buf_in_tmp = m_buf_in;
        memcpy( m_buf_in + m_byte_left_in , str_in, size_in );    
    }
    else m_buf_in_tmp = str_in;

    m_byte_left_in += size_in;

    // iconv 実行
    do{

#ifdef _DEBUG
        std::cout << "m_byte_left_in = " << m_byte_left_in << std::endl;
        std::cout << "byte_left_out = " << byte_left_out << std::endl;
#endif
    
        const int ret = g_iconv( m_cd, &m_buf_in_tmp, &m_byte_left_in, &buf_out, &byte_left_out );

#ifdef _DEBUG
        std::cout << "--> ret = " << ret << std::endl;
        std::cout << "m_byte_left_in = " << m_byte_left_in << std::endl;
        std::cout << "byte_left_out = " << byte_left_out << std::endl;
#endif

        //　エラー
        if( ret == -1 ){

            if( errno == EILSEQ ){

#ifdef _DEBUG_ICONV
                char str_tmp[256];
#endif
                const unsigned char code0 = *m_buf_in_tmp;
                const unsigned char code1 = *(m_buf_in_tmp+1);
                const unsigned char code2 = *(m_buf_in_tmp+2);

                if( m_coding_from == "MS932" )
                {

                    // 空白(0xa0)
                    if( code0 == 0xa0 ){
                        *m_buf_in_tmp = 0x20;
                        continue;
                    }

                    // <>の誤判別 ( 開発スレ 489 を参照 )
                    if( code1 == 0x3c && code2 == 0x3e ){
                        *m_buf_in_tmp = '?';
#ifdef _DEBUG_ICONV
                        snprintf( str_tmp, 256, "iconv 0x%x%x> -> ?<>", code0, code1 );
                        MISC::MSG( str_tmp );
#endif
                        continue;
                    }

                    // マッピング失敗
                    // □(0x81a0)を表示する
                    if( ( code0 >= 0x81 && code0 <=0x9F )
                        || ( code0 >= 0xe0 && code0 <=0xef ) ){

                        *m_buf_in_tmp = static_cast< char >( 0x81 );
                        *(m_buf_in_tmp+1) = static_cast< char >( 0xa0 );

#ifdef _DEBUG_ICONV
                        snprintf( str_tmp, 256, "iconv 0x%x%x -> □ (0x81a0) ", code0, code1 );
                        MISC::MSG( str_tmp );
#endif
                        continue;
                    }
                }

                // unicode 文字からの変換失敗
                // 数値文字参照(&#????;)形式にする
                if( m_coding_from == "UTF-8" ){

                    // https://github.com/JDimproved/JDim/issues/214 （emoji subdivision flagの処理）について
                    //
                    // TAG LATIN SMALL LETTER と CANCEL TAG については、libcのiconv()関数にかけると、
                    // エラーを返さず、なおかつこれらの文字を無視してしまうので、
                    // U+1F3F4 WAVING BLACK FLAGが現れた時は、上の範囲のUTF-8文字が続いてないか、
                    // このblockの範囲で確認してから、脱出する。


                    bool is_converted_to_ucs2 = false;  // 数値文字参照に変換されたかどうか
                    bool is_handling_emoji_subdivision_flag = false;  // emoji subdivision flags の処理の途中か

                    for ( ; ; ) {
                        int byte;
                        const int ucs2 = MISC::utf8toucs2( m_buf_in_tmp, byte );
                        if( byte <= 1 ) break;

                        // emoji subdivision flags の処理
                        if ( is_handling_emoji_subdivision_flag ) {
                            // Tag Latin Small Letterの範囲か、Cancel Tagでなければ、処理中断
                            if ( byte != 4 ) break;
                            if ( ucs2 < 917601 ) break; // U+E0061 TAG LATIN SMALL LETTER A
                            if ( ucs2 > 917631 ) break; // U+E007F CANCEL TAG
                        }

                        const std::string ucs2_str = std::to_string( ucs2 );
#ifdef _DEBUG
                        std::cout << "ucs2 = " << ucs2_str << " byte = " << byte << std::endl;
#endif
                        m_buf_in_tmp += byte;
                        m_byte_left_in -= byte;

                        *(buf_out++) = '&';
                        *(buf_out++) = '#';
                        memcpy( buf_out, ucs2_str.c_str(), ucs2_str.size() ); buf_out += ucs2_str.size();
                        *(buf_out++) = ';';

                        byte_left_out -= ucs2_str.size() + 3;
                        is_converted_to_ucs2 = true;  // 一度変換されたのでマーク

                        if ( ! is_handling_emoji_subdivision_flag ) {
                            if ( ( byte == 4 ) && ( ucs2 == 127988 ) ){ // U+1F3F4 WAVING BLACK FLAG
                                // emoji subdivision flags の処理開始
                                is_handling_emoji_subdivision_flag = true;
                                continue; // 連続処理
                            }
                        } else {
                            // まだ emoji subdivision flags の処理中
                            continue;
                        }

                        break;
                    }

                    // 数値文字参照に変換された場合は、continueする
                    if ( is_converted_to_ucs2 ) continue;

                }

                // 時々空白(0x20)で EILSEQ が出るときがあるのでもう一度トライする
                if( code0 == 0x20 ) continue;

                //その他、1文字を空白にして続行
#ifdef _DEBUG_ICONV
                snprintf( str_tmp, 256, "iconv EILSEQ left = %zu code = %x %x %x", m_byte_left_in, code0, code1, code2 );
                MISC::ERRMSG( str_tmp );
#endif
                *m_buf_in_tmp = 0x20;
            }

            else if( errno == EINVAL ){
#ifdef _DEBUG_ICONV
                MISC::ERRMSG( "iconv EINVAL\n" );
#endif
                break;
            }

            else if( errno == E2BIG ){
#ifdef _DEBUG_ICONV
                MISC::ERRMSG( "iconv E2BIG\n" );
#endif
                break;
            }
        }
    
    } while( m_byte_left_in > 0 );

    size_out = BUF_SIZE_ICONV_OUT - byte_left_out;
    m_buf_out[ size_out ] = '\0';
    
    return m_buf_out;
}

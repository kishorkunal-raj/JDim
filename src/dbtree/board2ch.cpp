// ライセンス: GPL2

//#define _DEBUG
#include "jddebug.h"

#include "board2ch.h"
#include "article2ch.h"
#include "articlehash.h"
#include "frontloader.h"

#include "config/globalconf.h"

#include "jdlib/miscutil.h"
#include "jdlib/misctime.h"

#include "login2ch.h"
#include "loginbe.h"

#include <sstream>

using namespace DBTREE;


Board2ch::Board2ch( const std::string& root, const std::string& path_board, const std::string& name )
    : Board2chCompati( root, path_board, name, std::string() )
{
#ifdef _DEBUG
    std::cout << "Board2ch::Board2ch\n";
#endif
}


Board2ch::~Board2ch() noexcept
{
    if( m_frontloader ) {
        m_frontloader->terminate_load();
    }
}


// ユーザエージェント
// ダウンロード用
const std::string& Board2ch::get_agent()
{
    return CONFIG::get_agent_for2ch();
}

// 書き込み用
const std::string& Board2ch::get_agent_w()
{
    return CONFIG::get_agent_for2ch();
}


// 読み込み用プロキシ
std::string Board2ch::get_proxy_host()
{
    const int mode = get_mode_local_proxy();

    if( mode == DBTREE::PROXY_GLOBAL ){

        if( CONFIG::get_use_proxy_for2ch() ) return CONFIG::get_proxy_for2ch();
    }
    else if( mode == DBTREE::PROXY_LOCAL ) return get_local_proxy();

    return std::string();
}

int Board2ch::get_proxy_port()
{
    const int mode = get_mode_local_proxy();

    if( mode == DBTREE::PROXY_GLOBAL ) return CONFIG::get_proxy_port_for2ch();
    else if( mode == DBTREE::PROXY_LOCAL ) return get_local_proxy_port();

    return 0;
}

std::string Board2ch::get_proxy_basicauth()
{
    const int mode = get_mode_local_proxy();

    if( mode == DBTREE::PROXY_GLOBAL ) return CONFIG::get_proxy_basicauth_for2ch();
    else if( mode == DBTREE::PROXY_LOCAL ) return get_local_proxy_basicauth();

    return std::string();
}


// 書き込み用プロキシ
std::string Board2ch::get_proxy_host_w()
{
    const int mode = get_mode_local_proxy_w();

    if( mode == DBTREE::PROXY_GLOBAL ){
        if( CONFIG::get_use_proxy_for2ch_w() ) return CONFIG::get_proxy_for2ch_w();
    }
    else if( mode == DBTREE::PROXY_LOCAL ) return get_local_proxy_w();

    return std::string();
}

int Board2ch::get_proxy_port_w()
{
    const int mode = get_mode_local_proxy_w();

    if( mode == DBTREE::PROXY_GLOBAL ) return CONFIG::get_proxy_port_for2ch_w();
    else if( mode == DBTREE::PROXY_LOCAL ) return get_local_proxy_port_w();

    return 0;
}


std::string Board2ch::get_proxy_basicauth_w()
{
    const int mode = get_mode_local_proxy_w();

    if( mode == DBTREE::PROXY_GLOBAL ) return CONFIG::get_proxy_basicauth_for2ch_w();
    else if( mode == DBTREE::PROXY_LOCAL ) return get_local_proxy_basicauth_w();

    return std::string();
}


// BE 用クッキー作成
void Board2ch::set_cookie_for_be( std::string& cookie ) const
{
    // BE ログイン中
    if( CORE::get_loginbe()->login_now() ) {
        if( ! cookie.empty() ) cookie += "; ";
        cookie += "DMDM=" + CORE::get_loginbe()->get_sessionid() + "; MDMD=" + CORE::get_loginbe()->get_sessiondata();
    }
}


//
// 読み込み用クッキー作成
//
// プロキシの2ch読み込み用設定がoffのとき
// またはプロキシにクッキーを送る設定のときは対象サイトにcookieを送信する
//
std::string Board2ch::cookie_for_request() const
{
    std::string cookie;

    if( ! CONFIG::get_use_proxy_for2ch() || CONFIG::get_send_cookie_to_proxy_for2ch() ) {
        cookie = cookie_by_host();
        if( cookie.empty() ) cookie = get_hap();
    }

    set_cookie_for_be( cookie );

#ifdef _DEBUG
    std::cout << "Board2ch::cookie_for_request cookie = " << cookie << std::endl;
#endif
    return cookie;
}


//
// 書き込み用クッキー作成
//
// プロキシの2ch書き込み用設定がoffのとき
// またはプロキシにクッキーを送る設定のときは対象サイトにcookieを送信する
//
std::string Board2ch::cookie_for_post() const
{
    std::string cookie;

    if( ! CONFIG::get_use_proxy_for2ch_w() || CONFIG::get_send_cookie_to_proxy_for2ch_w() ) {
        cookie = cookie_by_host();
        if( cookie.empty() ) cookie = get_hap();
    }

    set_cookie_for_be( cookie );

#ifdef _DEBUG
    std::cout << "Board2ch::cookie_for_post cookie = " << cookie << std::endl;
#endif
    return cookie;
}


std::string Board2ch::get_write_referer( const std::string& url )
{
    return url_readcgi( url, 0, 0 );
}


// フロントページのダウンロード
void Board2ch::download_front()
{
    if( ! m_frontloader ) m_frontloader.reset( new FrontLoader( url_boardbase() ) );
    m_frontloader->reset();
    m_frontloader->download_text();
}


// 新スレ作成時の書き込みメッセージ作成
std::string Board2ch::create_newarticle_message( const std::string& subject, const std::string& name,
                                                 const std::string& mail, const std::string& msg )
{
    if( subject.empty() ) return std::string();
    if( msg.empty() ) return std::string();
    if( ! m_frontloader || m_frontloader->get_data().empty() ) {
        // フロントページを読み込んでない場合はメッセージ作成を中断してダウンロードする
        set_keyword_for_newarticle( std::string{} );
        Board2ch::download_front();
        return {};
    }

    std::stringstream ss_post;
    ss_post << "submit="   << MISC::charset_url_encode( "新規スレッド作成", get_charset() )
            << "&subject=" << MISC::charset_url_encode( subject, get_charset() )
            << "&FROM="    << MISC::charset_url_encode( name, get_charset() )
            << "&mail="    << MISC::charset_url_encode( mail, get_charset() )
            << "&MESSAGE=" << MISC::charset_url_encode( msg, get_charset() )
            << "&bbs="     << get_id()
            << "&time="    << m_frontloader->get_time_modified();

    // キーワード
    const std::string keyword = get_keyword_for_newarticle();
    if( ! keyword.empty() ) ss_post << "&" << keyword;

    // 2chログイン中
    // sidを送る
    if( CORE::get_login2ch()->login_now() ){
        std::string sid = CORE::get_login2ch()->get_sessionid();
        ss_post << "&sid=" << MISC::url_encode( sid.c_str(), sid.length() );
    }

#ifdef _DEBUG
    std::cout << "Board2ch::create_newarticle_message " << ss_post.str() << std::endl;
#endif

    // スレ立てのメッセージを作成したらキーワードとフロントページの読み込み状況はリセットする
    set_keyword_for_newarticle( std::string{} );
    m_frontloader->reset();

    return ss_post.str();
}


//
// 新スレ作成時のbbscgi のURL
//
// (例) "http://www.hoge2ch.net/test/bbs.cgi"
//
//
std::string Board2ch::url_bbscgi_new()
{
    return Board2chCompati::url_bbscgi_new();
}


//
// 新スレ作成時のsubbbscgi のURL
//
// (例) "http://www.hoge2ch.net/test/subbbs.cgi"
//
std::string Board2ch::url_subbbscgi_new()
{
    return Board2chCompati::url_subbbscgi_new();
}



//
// 新しくArticleBaseクラスを追加してそのポインタを返す
//
// cached : HDD にキャッシュがあるならtrue
//
ArticleBase* Board2ch::append_article( const std::string& datbase, const std::string& id, const bool cached )
{
    if( empty() ) return get_article_null();

    ArticleBase* article = new DBTREE::Article2ch( datbase, id, cached );
    if( article ){
        get_hash_article()->push( article );

        // 最大レス数セット
        article->set_number_max( get_number_max_res() );
    }
    else return get_article_null();

    return article;
}



// 2chのクッキー
std::string Board2ch::get_hap() const
{
    if( ! CONFIG::get_use_cookie_hap() ) return std::string();

    if( get_root().find( ".bbspink.com" ) != std::string::npos ) return CONFIG::get_cookie_hap_bbspink();
    return CONFIG::get_cookie_hap();
}

void Board2ch::set_hap( const std::string& hap )
{
    if( ! CONFIG::get_use_cookie_hap() )  return;

    if( get_root().find( ".bbspink.com" ) != std::string::npos ) CONFIG::set_cookie_hap_bbspink( hap );
    else CONFIG::set_cookie_hap( hap );
}


//
// 2chのクッキーの更新
//
void Board2ch::update_hap()
{
    if( ! CONFIG::get_use_cookie_hap() ) return;

    const std::string new_cookie = Board2chCompati::cookie_for_request();

    if( ! new_cookie.empty() ) {
#ifdef _DEBUG
        const std::string old_cookie = get_hap();
#endif
        set_hap( new_cookie );
#ifdef _DEBUG
        std::cout << "Board2ch::update_hap old = " << old_cookie << std::endl;
        std::cout << "Board2ch::update_hap new = " << new_cookie << std::endl;
#endif
    }
}

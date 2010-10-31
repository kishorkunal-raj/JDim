// ライセンス: GPL2

// スレ一覧ビューの基底クラス

#ifndef _BOARDVIEWBASE_H
#define _BOARDVIEWBASE_H

#include "skeleton/view.h"
#include "skeleton/dragtreeview.h"

#include "columns.h"

#include <gtkmm.h>
#include <ctime>


namespace SKELETON
{
    class Admin;
}

namespace DBTREE
{
    class ArticleBase;
}

namespace BOARD
{

    class BoardViewBase : public SKELETON::View
    {
        // viewに表示するboardのURL ( SKELETON::View::m_url はview自身のURLなのに注意すること )
        std::string m_url_board;

        SKELETON::DragTreeView m_treeview;
        BOARD::TreeColumns m_columns;
        Glib::RefPtr< Gtk::ListStore > m_liststore;
        Gtk::ScrolledWindow m_scrwin;

        // 列
        Gtk::TreeView::Column* m_col_mark;
        Gtk::TreeView::Column* m_col_id;
        Gtk::TreeView::Column* m_col_board;
        Gtk::TreeView::Column* m_col_subject;
        Gtk::TreeView::Column* m_col_res;
        Gtk::TreeView::Column* m_col_str_load;        
        Gtk::TreeView::Column* m_col_str_new;
        Gtk::TreeView::Column* m_col_since;
        Gtk::TreeView::Column* m_col_write;
        Gtk::TreeView::Column* m_col_speed;
        Gtk::TreeView::Column* m_col_diff;

        // クリック状態
        bool m_clicked;

        // ダブルクリック状態
        bool m_dblclicked;

        // 押したキー
        int m_pressed_key;

        // ソートで使う変数
        int m_col;
        int m_previous_col;
        int m_sortmode;
        int m_previous_sortmode;

        // サーチで使う変数
        bool m_search_invert;
        std::string m_pre_query;
        
        // ポップアップメニュー用
        Gtk::TreeModel::Path m_path_selected;

        // ロード中
        bool m_loading;

        // ロード前の最終アクセス時刻 ( 新着判定用 )
        time_t m_last_access_time;

        // ロード中に draw_bg_articles() を呼び出したときに使う一時変数
        // draw_bg_articles() を参照せよ
        std::list< std::string > m_list_draw_bg_articles;

        // ポップアップメニュー表示のときにactivate_act_before_popupmenu()で使う変数
        bool m_enable_menuslot;

        // subject.txt をロードする
        bool m_load_subject_txt;

        // 先頭に「板」列を表示
        bool m_show_col_board;

        // 増分行が表示されている
        bool m_col_diff_is_shown;

    public:

        BoardViewBase( const std::string& url, const bool show_col_board );
        virtual ~BoardViewBase();

        const std::string& get_url_board() const { return m_url_board; }
        virtual const std::string url_for_copy();

        // 行数
        const int get_row_size();

        // SKELETON::View の関数のオーバロード

        virtual void save_session(){}

        virtual void update_url( const std::string& url_old, const std::string& url_new );

        virtual const int get_icon( const std::string& iconname );
        virtual const bool is_loading(){ return m_loading; }
        virtual const bool set_command( const std::string& command,
                                        const std::string& arg1 = std::string(),
                                        const std::string& arg2 = std::string()
            );

        virtual void clock_in();

        // キーを押した        
        virtual const bool slot_key_press( GdkEventKey* event );

        virtual void write();
        virtual void stop();
        virtual void show_view();
        virtual void redraw_scrollbar();
        virtual void relayout();
        virtual void focus_view();
        virtual void focus_out();
        virtual void close_view();
        virtual void delete_view();
        virtual void set_favorite();


        // 特定の行だけの表示内容更新
        // url : subject.txt のアドレス
        // id : DAT の ID(拡張子付き)
        // もし ID が empty() なら全ての行の表示内容を更新する
        virtual void update_item( const std::string& url, const std::string& id );

        virtual const bool operate_view( const int control );
        virtual void goto_top();
        virtual void goto_bottom();
        virtual void goto_num( const int num_to, const int num_from );
        virtual void scroll_left();
        virtual void scroll_right();
        virtual void show_preference();

        // 進む、戻る
        virtual void back_viewhistory( const int count );
        virtual void forward_viewhistory( const int count );

        // 検索
        virtual void exec_search();
        virtual void up_search();
        virtual void down_search();
        virtual void operate_search( const std::string& controlid );
        virtual void set_search_query( const std::string& query );

        void row_up();
        void row_down();
        void page_up();
        void page_down();

      protected:

        // 自動ソート抑制
        void unsorted_column();

        // url から row を取得
        Gtk::TreeModel::Row get_row_from_url( const std::string& url );

        SKELETON::DragTreeView& get_treeview(){ return m_treeview; }

        // Viewが所属するAdminクラス
        virtual SKELETON::Admin* get_admin();

        // ポップアップメニューを表示する前にメニューのアクティブ状態を切り替える
        virtual void activate_act_before_popupmenu( const std::string& url );

        // ポップアップメニュー取得
        virtual Gtk::Menu* get_popupmenu( const std::string& url );

        // view更新
        void update_view_impl( const std::vector< DBTREE::ArticleBase* >& list_article, const bool loading_fin );

        // ステータスバー更新
        void update_status();

        // subject.txt をロードする
        void set_load_subject_txt( const bool load ){ m_load_subject_txt = load; }

        // 行を作って内容をセット
        const Gtk::TreeModel::Row prepend_row( DBTREE::ArticleBase* art, const int id );

        // デフォルトのソート状態
        virtual const int get_default_sort_column();
        virtual const int get_default_view_sort_mode();
        virtual const int get_default_view_sort_pre_column();
        virtual const int get_default_view_sort_pre_mode();

    private:

        void setup_action();

        // 通常の右クリックメニューの作成
        const std::string create_context_menu();
        const char* get_menu_item( const int item );

        // 次スレ移行処理に使用する前スレのアドレス
        // BOARD::BoardViewNext と BoardViewBase::open_row()を参照せよ
        virtual const std::string get_url_pre_article(){ return std::string(); }

        void update_columns();

        const int get_title_id( const int col );

        // ソート列やソートモードの保存
        virtual void save_sort_columns();

        // 列の幅の保存
        virtual void save_column_width();

        void slot_cell_data( Gtk::CellRenderer* cell, const Gtk::TreeModel::iterator& it );

        // 全ての行の表示内容更新
        void update_item_all();

        // ソート実行
        void exec_sort();

        // ソート状態回復
        void restore_sort();

        // ヘッダをクリックしたときのslot関数
        void slot_col_clicked( const int col );

        const int compare_drawbg( Gtk::TreeModel::Row& row_a, Gtk::TreeModel::Row& row_b );
        const int compare_col( const int col, const int sortmode, Gtk::TreeModel::Row& row_a, Gtk::TreeModel::Row& row_b );
        const int slot_compare_row( const Gtk::TreeModel::iterator& a, const Gtk::TreeModel::iterator& b );

        // UI
        const bool slot_button_press( GdkEventButton* event );
        const bool slot_button_release( GdkEventButton* event );
        const bool slot_motion_notify( GdkEventMotion* event );
        const bool slot_key_release( GdkEventKey* event );
        const bool slot_scroll_event( GdkEventScroll* event );

        void slot_bookmark( int bookmark );
        void slot_open_tab();
        void slot_reget_article();
        void slot_favorite_thread();
        void slot_favorite_board();
        void slot_copy_url();
        void slot_copy_title_url();
        void slot_select_all();
        void slot_open_browser();
        void slot_preferences_article();
        void slot_save_dat();
        void slot_search_next();
        virtual void slot_abone_thread();
        void slot_delete_logs();

        // ドラッグアンドドロップ
        void slot_drag_data_get( const Glib::RefPtr<Gdk::DragContext>& context,
                                 Gtk::SelectionData& selection_data, guint info, guint time );
        void slot_dropped_url_list( const std::list< std::string >& );

        const bool open_row( Gtk::TreePath& path, const bool tab, const bool reget );
        void open_selected_rows( const bool reget );
        const std::string path2daturl( const Gtk::TreePath& path );
        const std::string path2url_board( const Gtk::TreePath& path );

        // 検索
        const bool drawout();

        void update_row_common( const Gtk::TreeModel::Row& row );
        const std::string get_subject_from_path( Gtk::TreePath& path );

        template < typename ColumnType >
        const std::string get_name_of_cell( Gtk::TreePath& path, const Gtk::TreeModelColumn< ColumnType >& column );

        void set_article_to_buffer();
        void set_board_to_buffer();

        // 指定したスレを強調して表示
        // dat 落ち等で表示されていないスレも強制的に表示する
        // 共有バッファに表示したいスレをセットしてから set_command 経由で呼び出す
        void draw_bg_articles();
    };

};


#endif

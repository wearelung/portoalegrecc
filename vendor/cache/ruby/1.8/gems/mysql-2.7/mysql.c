/*	ruby mysql module
 *	$Id: mysql.c.in,v 1.30 2005/08/21 15:15:38 tommy Exp $
 */

#include "ruby.h"
#include "version.h"
#ifdef HAVE_MYSQL_H
#include <mysql.h>
#include <errmsg.h>
#include <mysqld_error.h>
#else
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include <mysql/mysqld_error.h>
#endif

#define MYSQL_RUBY_VERSION 20700

#define GC_STORE_RESULT_LIMIT 20

#ifndef Qtrue		/* ruby 1.2.x ? */
#define	Qtrue		TRUE
#define	Qfalse		FALSE
#define	rb_exc_raise	rb_raise
#define	rb_exc_new2	exc_new2
#define	rb_str_new	str_new
#define	rb_str_new2	str_new2
#define	rb_ary_new2	ary_new2
#define	rb_ary_store	ary_store
#define	rb_obj_alloc	obj_alloc
#define	rb_hash_new	hash_new
#define	rb_hash_aset	hash_aset
#define	rb_eStandardError	eStandardError
#define	rb_cObject	cObject
#endif

#if MYSQL_VERSION_ID < 32224
#define	mysql_field_count	mysql_num_fields
#endif

#define NILorSTRING(obj)	(NIL_P(obj)? NULL: StringValuePtr(obj))
#define NILorINT(obj)		(NIL_P(obj)? 0: NUM2INT(obj))

#define GetMysqlStruct(obj)	(Check_Type(obj, T_DATA), (struct mysql*)DATA_PTR(obj))
#define GetHandler(obj)		(Check_Type(obj, T_DATA), &(((struct mysql*)DATA_PTR(obj))->handler))
#define GetMysqlRes(obj)	(Check_Type(obj, T_DATA), ((struct mysql_res*)DATA_PTR(obj))->res)
#define GetMysqlStmt(obj)	(Check_Type(obj, T_DATA), ((struct mysql_stmt*)DATA_PTR(obj))->stmt)

VALUE cMysql;
VALUE cMysqlRes;
VALUE cMysqlField;
VALUE cMysqlStmt;
VALUE cMysqlRowOffset;
VALUE cMysqlTime;
VALUE eMysql;

static int store_result_count = 0;

struct mysql {
    MYSQL handler;
    char connection;
    char query_with_result;
};

struct mysql_res {
    MYSQL_RES* res;
    char freed;
};

#if MYSQL_VERSION_ID >= 40101
struct mysql_stmt {
    MYSQL_STMT *stmt;
    char closed;
    struct {
	int n;
	MYSQL_BIND *bind;
	unsigned long *length;
	MYSQL_TIME *buffer;
    } param;
    struct {
	int n;
	MYSQL_BIND *bind;
	my_bool *is_null;
	unsigned long *length;
    } result;
    MYSQL_RES *res;
};
#endif

/*	free Mysql class object		*/
static void free_mysql(struct mysql* my)
{
    if (my->connection == Qtrue)
	mysql_close(&my->handler);
    free(my);
}

static void free_mysqlres(struct mysql_res* resp)
{
    if (resp->freed == Qfalse) {
	mysql_free_result(resp->res);
	store_result_count--;
    }
    free(resp);
}

#if MYSQL_VERSION_ID >= 40101
static void free_mysqlstmt_memory(struct mysql_stmt *s)
{
    if (s->param.bind) {
	xfree(s->param.bind);
	s->param.bind = NULL;
    }
    if (s->param.length) {
	xfree(s->param.length);
	s->param.length = NULL;
    }
    if (s->param.buffer) {
	xfree(s->param.buffer);
	s->param.buffer = NULL;
    }
    s->param.n = 0;
    if (s->res) {
	mysql_free_result(s->res);
	s->res = NULL;
    }
    if (s->result.bind) {
	int i;
	for (i = 0; i < s->result.n; i++) {
	    if (s->result.bind[i].buffer)
		xfree(s->result.bind[i].buffer);
	}
	xfree(s->result.bind);
	s->result.bind = NULL;
    }
    if (s->result.is_null) {
	xfree(s->result.is_null);
	s->result.is_null = NULL;
    }
    if (s->result.length) {
	xfree(s->result.length);
	s->result.length = NULL;
    }
    s->result.n = 0;
}

static void free_mysqlstmt(struct mysql_stmt* s)
{
    free_mysqlstmt_memory(s);
    if (s->closed == Qfalse)
	mysql_stmt_close(s->stmt);
    if (s->res)
	mysql_free_result(s->res);
    free(s);
}
#endif

static void mysql_raise(MYSQL* m)
{
    VALUE e = rb_exc_new2(eMysql, mysql_error(m));
    rb_iv_set(e, "errno", INT2FIX(mysql_errno(m)));
#if MYSQL_VERSION_ID >= 40101
    rb_iv_set(e, "sqlstate", rb_tainted_str_new2(mysql_sqlstate(m)));
#endif
    rb_exc_raise(e);
}

static VALUE mysqlres2obj(MYSQL_RES* res)
{
    VALUE obj;
    struct mysql_res* resp;
    obj = Data_Make_Struct(cMysqlRes, struct mysql_res, 0, free_mysqlres, resp);
    resp->res = res;
    resp->freed = Qfalse;
    rb_obj_call_init(obj, 0, NULL);
    if (++store_result_count > GC_STORE_RESULT_LIMIT)
	rb_gc();
    return obj;
}

/*	make Mysql::Field object	*/
static VALUE make_field_obj(MYSQL_FIELD* f)
{
    VALUE obj;
    VALUE hash;
    if (f == NULL)
	return Qnil;
    obj = rb_obj_alloc(cMysqlField);
    rb_iv_set(obj, "name", f->name? rb_str_freeze(rb_tainted_str_new2(f->name)): Qnil);
    rb_iv_set(obj, "table", f->table? rb_str_freeze(rb_tainted_str_new2(f->table)): Qnil);
    rb_iv_set(obj, "def", f->def? rb_str_freeze(rb_tainted_str_new2(f->def)): Qnil);
    rb_iv_set(obj, "type", INT2NUM(f->type));
    rb_iv_set(obj, "length", INT2NUM(f->length));
    rb_iv_set(obj, "max_length", INT2NUM(f->max_length));
    rb_iv_set(obj, "flags", INT2NUM(f->flags));
    rb_iv_set(obj, "decimals", INT2NUM(f->decimals));
    return obj;
}

/*-------------------------------
 * Mysql class method
 */

/*	init()	*/
static VALUE init(VALUE klass)
{
    struct mysql* myp;
    VALUE obj;

    obj = Data_Make_Struct(klass, struct mysql, 0, free_mysql, myp);
    mysql_init(&myp->handler);
    myp->connection = Qfalse;
    myp->query_with_result = Qtrue;
    rb_obj_call_init(obj, 0, NULL);
    return obj;
}

/*	real_connect(host=nil, user=nil, passwd=nil, db=nil, port=nil, sock=nil, flag=nil)	*/
static VALUE real_connect(int argc, VALUE* argv, VALUE klass)
{
    VALUE host, user, passwd, db, port, sock, flag;
    char *h, *u, *p, *d, *s;
    unsigned int pp, f;
    struct mysql* myp;
    VALUE obj;

#if MYSQL_VERSION_ID >= 32200
    rb_scan_args(argc, argv, "07", &host, &user, &passwd, &db, &port, &sock, &flag);
    d = NILorSTRING(db);
    f = NILorINT(flag);
#elif MYSQL_VERSION_ID >= 32115
    rb_scan_args(argc, argv, "06", &host, &user, &passwd, &port, &sock, &flag);
    f = NILorINT(flag);
#else
    rb_scan_args(argc, argv, "05", &host, &user, &passwd, &port, &sock);
#endif
    h = NILorSTRING(host);
    u = NILorSTRING(user);
    p = NILorSTRING(passwd);
    pp = NILorINT(port);
    s = NILorSTRING(sock);

    obj = Data_Make_Struct(klass, struct mysql, 0, free_mysql, myp);
#if MYSQL_VERSION_ID >= 32200
    mysql_init(&myp->handler);
    if (mysql_real_connect(&myp->handler, h, u, p, d, pp, s, f) == NULL)
#elif MYSQL_VERSION_ID >= 32115
    if (mysql_real_connect(&myp->handler, h, u, p, pp, s, f) == NULL)
#else
    if (mysql_real_connect(&myp->handler, h, u, p, pp, s) == NULL)
#endif
	mysql_raise(&myp->handler);

    myp->handler.reconnect = 0;
    myp->connection = Qtrue;
    myp->query_with_result = Qtrue;
    rb_obj_call_init(obj, argc, argv);

    return obj;
}

/*	escape_string(string)	*/
static VALUE escape_string(VALUE klass, VALUE str)
{
    VALUE ret;
    Check_Type(str, T_STRING);
    ret = rb_str_new(0, (RSTRING(str)->len)*2+1);
    RSTRING(ret)->len = mysql_escape_string(RSTRING(ret)->ptr, RSTRING(str)->ptr, RSTRING(str)->len);
    return ret;
}

/*	client_info()	*/
static VALUE client_info(VALUE klass)
{
    return rb_tainted_str_new2(mysql_get_client_info());
}

#if MYSQL_VERSION_ID >= 32332
/*	my_debug(string)	*/
static VALUE my_debug(VALUE obj, VALUE str)
{
    mysql_debug(StringValuePtr(str));
    return obj;
}
#endif

#if MYSQL_VERSION_ID >= 40000
/*	client_version()	*/
static VALUE client_version(VALUE obj)
{
    return INT2NUM(mysql_get_client_version());
}
#endif

/*-------------------------------
 * Mysql object method
 */

#if MYSQL_VERSION_ID >= 32200
/*	real_connect(host=nil, user=nil, passwd=nil, db=nil, port=nil, sock=nil, flag=nil)	*/
static VALUE real_connect2(int argc, VALUE* argv, VALUE obj)
{
    VALUE host, user, passwd, db, port, sock, flag;
    char *h, *u, *p, *d, *s;
    unsigned int pp, f;
    MYSQL* m = GetHandler(obj);
    rb_scan_args(argc, argv, "07", &host, &user, &passwd, &db, &port, &sock, &flag);
    d = NILorSTRING(db);
    f = NILorINT(flag);
    h = NILorSTRING(host);
    u = NILorSTRING(user);
    p = NILorSTRING(passwd);
    pp = NILorINT(port);
    s = NILorSTRING(sock);

    if (mysql_real_connect(m, h, u, p, d, pp, s, f) == NULL)
	mysql_raise(m);
    m->reconnect = 0;
    GetMysqlStruct(obj)->connection = Qtrue;

    return obj;
}

/*	options(opt, value=nil)	*/
static VALUE options(int argc, VALUE* argv, VALUE obj)
{
    VALUE opt, val;
    int n;
    my_bool b;
    char* v;
    MYSQL* m = GetHandler(obj);

    rb_scan_args(argc, argv, "11", &opt, &val);
    switch(NUM2INT(opt)) {
    case MYSQL_OPT_CONNECT_TIMEOUT:
#if MYSQL_VERSION_ID >= 40100
    case MYSQL_OPT_PROTOCOL:
#endif
#if MYSQL_VERSION_ID >= 40101
    case MYSQL_OPT_READ_TIMEOUT:
    case MYSQL_OPT_WRITE_TIMEOUT:
#endif
	if (val == Qnil)
	    rb_raise(rb_eArgError, "wrong # of arguments(1 for 2)");
	n = NUM2INT(val);
	v = (char*)&n;
	break;
    case MYSQL_INIT_COMMAND:
    case MYSQL_READ_DEFAULT_FILE:
    case MYSQL_READ_DEFAULT_GROUP:
#if MYSQL_VERSION_ID >= 32349
    case MYSQL_SET_CHARSET_DIR:
    case MYSQL_SET_CHARSET_NAME:
#endif
#if MYSQL_VERSION_ID >= 40100
    case MYSQL_SHARED_MEMORY_BASE_NAME:
#endif
#if MYSQL_VERSION_ID >= 40101
    case MYSQL_SET_CLIENT_IP:
#endif
	if (val == Qnil)
	    rb_raise(rb_eArgError, "wrong # of arguments(1 for 2)");
	v = StringValuePtr(val);
	break;
#if MYSQL_VERSION_ID >= 40101
    case MYSQL_SECURE_AUTH:
	if (val == Qnil || val == Qfalse)
	    b = 1;
	else
	    b = 0;
	v = (char*)&b;
	break;
#endif
#if MYSQL_VERSION_ID >= 32349
    case MYSQL_OPT_LOCAL_INFILE:
	if (val == Qnil || val == Qfalse)
	    v = NULL;
	else {
	    n = 1;
	    v = (char*)&n;
	}
	break;
#endif
    default:
	v = NULL;
    }

    if (mysql_options(m, NUM2INT(opt), v) != 0)
	rb_raise(eMysql, "unknown option: %d", NUM2INT(opt));
    return obj;
}
#endif

#if MYSQL_VERSION_ID >= 32332
/*	real_escape_string(string)	*/
static VALUE real_escape_string(VALUE obj, VALUE str)
{
    MYSQL* m = GetHandler(obj);
    VALUE ret;
    Check_Type(str, T_STRING);
    ret = rb_str_new(0, (RSTRING(str)->len)*2+1);
    RSTRING(ret)->len = mysql_real_escape_string(m, RSTRING(ret)->ptr, RSTRING(str)->ptr, RSTRING(str)->len);
    return ret;
}
#endif

/*	initialize()	*/
static VALUE initialize(int argc, VALUE* argv, VALUE obj)
{
    return obj;
}

/*	affected_rows()	*/
static VALUE affected_rows(VALUE obj)
{
    return INT2NUM(mysql_affected_rows(GetHandler(obj)));
}

#if MYSQL_VERSION_ID >= 32303
/*	change_user(user=nil, passwd=nil, db=nil)	*/
static VALUE change_user(int argc, VALUE* argv, VALUE obj)
{
    VALUE user, passwd, db;
    char *u, *p, *d;
    MYSQL* m = GetHandler(obj);
    rb_scan_args(argc, argv, "03", &user, &passwd, &db);
    u = NILorSTRING(user);
    p = NILorSTRING(passwd);
    d = NILorSTRING(db);
    if (mysql_change_user(m, u, p, d) != 0)
	mysql_raise(m);
    return obj;
}
#endif

#if MYSQL_VERSION_ID >= 32321
/*	character_set_name()	*/
static VALUE character_set_name(VALUE obj)
{
    return rb_tainted_str_new2(mysql_character_set_name(GetHandler(obj)));
}
#endif

/*	close()		*/
static VALUE my_close(VALUE obj)
{
    MYSQL* m = GetHandler(obj);
    mysql_close(m);
    if (mysql_errno(m))
	mysql_raise(m);
    GetMysqlStruct(obj)->connection = Qfalse;
    return obj;
}

#if MYSQL_VERSION_ID < 40000
/*	create_db(db)	*/
static VALUE create_db(VALUE obj, VALUE db)
{
    MYSQL* m = GetHandler(obj);
    if (mysql_create_db(m, StringValuePtr(db)) != 0)
	mysql_raise(m);
    return obj;
}

/*	drop_db(db)	*/
static VALUE drop_db(VALUE obj, VALUE db)
{
    MYSQL* m = GetHandler(obj);
    if (mysql_drop_db(m, StringValuePtr(db)) != 0)
	mysql_raise(m);
    return obj;
}
#endif

#if MYSQL_VERSION_ID >= 32332
/*	dump_debug_info()	*/
static VALUE dump_debug_info(VALUE obj)
{
    MYSQL* m = GetHandler(obj);
    if (mysql_dump_debug_info(m) != 0)
	mysql_raise(m);
    return obj;
}
#endif

/*	errno()		*/
static VALUE my_errno(VALUE obj)
{
    return INT2NUM(mysql_errno(GetHandler(obj)));
}

/*	error()		*/
static VALUE my_error(VALUE obj)
{
    return rb_str_new2(mysql_error(GetHandler(obj)));
}

/*	field_count()	*/
static VALUE field_count(VALUE obj)
{
    return INT2NUM(mysql_field_count(GetHandler(obj)));
}

/*	host_info()	*/
static VALUE host_info(VALUE obj)
{
    return rb_tainted_str_new2(mysql_get_host_info(GetHandler(obj)));
}

/*	proto_info()	*/
static VALUE proto_info(VALUE obj)
{
    return INT2NUM(mysql_get_proto_info(GetHandler(obj)));
}

/*	server_info()	*/
static VALUE server_info(VALUE obj)
{
    return rb_tainted_str_new2(mysql_get_server_info(GetHandler(obj)));
}

/*	info()		*/
static VALUE info(VALUE obj)
{
    const char* p = mysql_info(GetHandler(obj));
    return p? rb_tainted_str_new2(p): Qnil;
}

/*	insert_id()	*/
static VALUE insert_id(VALUE obj)
{
    return INT2NUM(mysql_insert_id(GetHandler(obj)));
}

/*	kill(pid)	*/
static VALUE my_kill(VALUE obj, VALUE pid)
{
    int p = NUM2INT(pid);
    MYSQL* m = GetHandler(obj);
    if (mysql_kill(m, p) != 0)
	mysql_raise(m);
    return obj;
}

/*	list_dbs(db=nil)	*/
static VALUE list_dbs(int argc, VALUE* argv, VALUE obj)
{
    unsigned int i, n;
    VALUE db, ret;
    MYSQL* m = GetHandler(obj);
    MYSQL_RES* res;

    rb_scan_args(argc, argv, "01", &db);
    res = mysql_list_dbs(m, NILorSTRING(db));
    if (res == NULL)
	mysql_raise(m);

    n = mysql_num_rows(res);
    ret = rb_ary_new2(n);
    for (i=0; i<n; i++)
	rb_ary_store(ret, i, rb_tainted_str_new2(mysql_fetch_row(res)[0]));
    mysql_free_result(res);
    return ret;
}

/*	list_fields(table, field=nil)	*/
static VALUE list_fields(int argc, VALUE* argv, VALUE obj)
{
    VALUE table, field;
    MYSQL* m = GetHandler(obj);
    MYSQL_RES* res;
    rb_scan_args(argc, argv, "11", &table, &field);
    res = mysql_list_fields(m, StringValuePtr(table), NILorSTRING(field));
    if (res == NULL)
	mysql_raise(m);
    return mysqlres2obj(res);
}

/*	list_processes()	*/
static VALUE list_processes(VALUE obj)
{
    MYSQL* m = GetHandler(obj);
    MYSQL_RES* res = mysql_list_processes(m);
    if (res == NULL)
	mysql_raise(m);
    return mysqlres2obj(res);
}

/*	list_tables(table=nil)	*/
static VALUE list_tables(int argc, VALUE* argv, VALUE obj)
{
    VALUE table;
    MYSQL* m = GetHandler(obj);
    MYSQL_RES* res;
    unsigned int i, n;
    VALUE ret;

    rb_scan_args(argc, argv, "01", &table);
    res = mysql_list_tables(m, NILorSTRING(table));
    if (res == NULL)
	mysql_raise(m);

    n = mysql_num_rows(res);
    ret = rb_ary_new2(n);
    for (i=0; i<n; i++)
	rb_ary_store(ret, i, rb_tainted_str_new2(mysql_fetch_row(res)[0]));
    mysql_free_result(res);
    return ret;
}

/*	ping()		*/
static VALUE ping(VALUE obj)
{
    MYSQL* m = GetHandler(obj);
    if (mysql_ping(m) != 0)
	mysql_raise(m);
    return obj;
}

/*	refresh(r)	*/
static VALUE refresh(VALUE obj, VALUE r)
{
    MYSQL* m = GetHandler(obj);
    if (mysql_refresh(m, NUM2INT(r)) != 0)
	mysql_raise(m);
    return obj;
}

/*	reload()	*/
static VALUE reload(VALUE obj)
{
    MYSQL* m = GetHandler(obj);
    if (mysql_reload(m) != 0)
	mysql_raise(m);
    return obj;
}

/*	select_db(db)	*/
static VALUE select_db(VALUE obj, VALUE db)
{
    MYSQL* m = GetHandler(obj);
    if (mysql_select_db(m, StringValuePtr(db)) != 0)
	mysql_raise(m);
    return obj;
}

/*	shutdown()	*/
static VALUE my_shutdown(int argc, VALUE* argv, VALUE obj)
{
    MYSQL* m = GetHandler(obj);
    VALUE level;

    rb_scan_args(argc, argv, "01", &level);
#if MYSQL_VERSION_ID >= 40103
    if (mysql_shutdown(m, NIL_P(level) ? SHUTDOWN_DEFAULT : NUM2INT(level)) != 0)
#else
    if (mysql_shutdown(m) != 0)
#endif
	mysql_raise(m);
    return obj;
}

/*	stat()		*/
static VALUE my_stat(VALUE obj)
{
    MYSQL* m = GetHandler(obj);
    const char* s = mysql_stat(m);
    if (s == NULL)
	mysql_raise(m);
    return rb_tainted_str_new2(s);
}

/*	store_result()	*/
static VALUE store_result(VALUE obj)
{
    MYSQL* m = GetHandler(obj);
    MYSQL_RES* res = mysql_store_result(m);
    if (res == NULL)
	mysql_raise(m);
    return mysqlres2obj(res);
}

/*	thread_id()	*/
static VALUE thread_id(VALUE obj)
{
    return INT2NUM(mysql_thread_id(GetHandler(obj)));
}

/*	use_result()	*/
static VALUE use_result(VALUE obj)
{
    MYSQL* m = GetHandler(obj);
    MYSQL_RES* res = mysql_use_result(m);
    if (res == NULL)
	mysql_raise(m);
    return mysqlres2obj(res);
}

static VALUE res_free(VALUE);
/*	query(sql)	*/
static VALUE query(VALUE obj, VALUE sql)
{
    MYSQL* m = GetHandler(obj);
    Check_Type(sql, T_STRING);
    if (rb_block_given_p()) {
#if MYSQL_VERSION_ID >= 40101
        if (mysql_get_server_version(m) >= 40101 && mysql_set_server_option(m, MYSQL_OPTION_MULTI_STATEMENTS_ON) != 0)
	    mysql_raise(m);
#endif
	if (mysql_real_query(m, RSTRING(sql)->ptr, RSTRING(sql)->len) != 0)
	    mysql_raise(m);
	do {
	    MYSQL_RES* res = mysql_store_result(m);
	    if (res == NULL) {
		if (mysql_field_count(m) != 0)
		    mysql_raise(m);
	    } else {
		VALUE robj = mysqlres2obj(res);
		rb_ensure(rb_yield, robj, res_free, robj);
	    }
	}
#if MYSQL_VERSION_ID >= 40101
	while (mysql_next_result(m) == 0);
#else
	while (0);
#endif
	return obj;
    }
    if (mysql_real_query(m, RSTRING(sql)->ptr, RSTRING(sql)->len) != 0)
	mysql_raise(m);
    if (GetMysqlStruct(obj)->query_with_result == Qfalse)
	return obj;
    if (mysql_field_count(m) == 0)
	return Qnil;
    return store_result(obj);
}

#if MYSQL_VERSION_ID >= 40100
/*	server_version()	*/
static VALUE server_version(VALUE obj)
{
    return INT2NUM(mysql_get_server_version(GetHandler(obj)));
}

/*	warning_count()	*/
static VALUE warning_count(VALUE obj)
{
    return INT2NUM(mysql_warning_count(GetHandler(obj)));
}

/*	commit()	*/
static VALUE commit(VALUE obj)
{
    MYSQL* m = GetHandler(obj);
    if (mysql_commit(m) != 0)
        mysql_raise(m);
    return obj;
}

/*	rollback()	*/
static VALUE rollback(VALUE obj)
{
    MYSQL* m = GetHandler(obj);
    if (mysql_rollback(m) != 0)
        mysql_raise(m);
    return obj;
}

/*	autocommit()	*/
static VALUE autocommit(VALUE obj, VALUE mode)
{
    MYSQL* m = GetHandler(obj);
    int f;
    f = (mode == Qnil || mode == Qfalse || (rb_type(mode) == T_FIXNUM && NUM2INT(mode) == 0)) ? 0 : 1;
    if (mysql_autocommit(m, f) != 0)
        mysql_raise(m);
    return obj;
}
#endif

#ifdef HAVE_MYSQL_SSL_SET
/*	ssl_set(key=nil, cert=nil, ca=nil, capath=nil, cipher=nil)	*/
static VALUE ssl_set(int argc, VALUE* argv, VALUE obj)
{
    VALUE key, cert, ca, capath, cipher;
    char *s_key, *s_cert, *s_ca, *s_capath, *s_cipher;
    MYSQL* m = GetHandler(obj);
    rb_scan_args(argc, argv, "05", &key, &cert, &ca, &capath, &cipher);
    s_key = NILorSTRING(key);
    s_cert = NILorSTRING(cert);
    s_ca = NILorSTRING(ca);
    s_capath = NILorSTRING(capath);
    s_cipher = NILorSTRING(cipher);
    mysql_ssl_set(m, s_key, s_cert, s_ca, s_capath, s_cipher);
    return obj;
}
#endif

#if MYSQL_VERSION_ID >= 40100
/*	more_results()		*/
static VALUE more_results(VALUE obj)
{
    if (mysql_more_results(GetHandler(obj)) == 0)
	return Qfalse;
    else
	return Qtrue;
}

static VALUE next_result(VALUE obj)
{
    MYSQL* m = GetHandler(obj);
    int ret;
    ret = mysql_next_result(m);
    if (ret > 0)
	mysql_raise(m);
    if (ret == 0)
	return Qtrue;
    return Qfalse;
}
#endif

#if MYSQL_VERSION_ID >= 40101
/*	set_server_option(option)	*/
static VALUE set_server_option(VALUE obj, VALUE option)
{
    MYSQL *m = GetHandler(obj);
    if (mysql_set_server_option(m, NUM2INT(option)) != 0)
	mysql_raise(m);
    return obj;
}

/*	sqlstate()	*/
static VALUE sqlstate(VALUE obj)
{
    MYSQL *m = GetHandler(obj);
    return rb_tainted_str_new2(mysql_sqlstate(m));
}
#endif

#if MYSQL_VERSION_ID >= 40102
/*	stmt_init()	*/
static VALUE stmt_init(VALUE obj)
{
    MYSQL *m = GetHandler(obj);
    MYSQL_STMT *s;
    struct mysql_stmt* stmt;
    my_bool true = 1;
    VALUE st_obj;

    if ((s = mysql_stmt_init(m)) == NULL)
	mysql_raise(m);
    if (mysql_stmt_attr_set(s, STMT_ATTR_UPDATE_MAX_LENGTH, &true))
	rb_raise(rb_eArgError, "mysql_stmt_attr_set() failed");
    st_obj = Data_Make_Struct(cMysqlStmt, struct mysql_stmt, 0, free_mysqlstmt, stmt);
    memset(stmt, 0, sizeof(*stmt));
    stmt->stmt = s;
    stmt->closed = Qfalse;
    return st_obj;
}

static VALUE stmt_prepare(VALUE obj, VALUE query);
/*	prepare(query)	*/
static VALUE prepare(VALUE obj, VALUE query)
{
    VALUE st;
    st = stmt_init(obj);
    return stmt_prepare(st, query);
}
#endif

/*	query_with_result()	*/
static VALUE query_with_result(VALUE obj)
{
    return GetMysqlStruct(obj)->query_with_result? Qtrue: Qfalse;
}

/*	query_with_result=(flag)	*/
static VALUE query_with_result_set(VALUE obj, VALUE flag)
{
    if (TYPE(flag) != T_TRUE && TYPE(flag) != T_FALSE)
#if RUBY_VERSION_CODE < 160
	TypeError("invalid type, required true or false.");
#else
        rb_raise(rb_eTypeError, "invalid type, required true or false.");
#endif
    GetMysqlStruct(obj)->query_with_result = flag;
    return flag;
}

/*	reconnect()	*/
static VALUE reconnect(VALUE obj)
{
    return GetHandler(obj)->reconnect ? Qtrue : Qfalse;
}

/*	reconnect=(flag)	*/
static VALUE reconnect_set(VALUE obj, VALUE flag)
{
    GetHandler(obj)->reconnect = (flag == Qnil || flag == Qfalse) ? 0 : 1;
    return flag;
}

/*-------------------------------
 * Mysql::Result object method
 */

/*	check if already freed	*/
static void check_free(VALUE obj)
{
    struct mysql_res* resp = DATA_PTR(obj);
    if (resp->freed == Qtrue)
        rb_raise(eMysql, "Mysql::Result object is already freed");
}

/*	data_seek(offset)	*/
static VALUE data_seek(VALUE obj, VALUE offset)
{
    check_free(obj);
    mysql_data_seek(GetMysqlRes(obj), NUM2INT(offset));
    return obj;
}

/*	fetch_field()	*/
static VALUE fetch_field(VALUE obj)
{
    check_free(obj);
    return make_field_obj(mysql_fetch_field(GetMysqlRes(obj)));
}

/*	fetch_fields()	*/
static VALUE fetch_fields(VALUE obj)
{
    MYSQL_RES* res;
    MYSQL_FIELD* f;
    unsigned int n;
    VALUE ret;
    unsigned int i;
    check_free(obj);
    res = GetMysqlRes(obj);
    f = mysql_fetch_fields(res);
    n = mysql_num_fields(res);
    ret = rb_ary_new2(n);
    for (i=0; i<n; i++)
	rb_ary_store(ret, i, make_field_obj(&f[i]));
    return ret;
}

/*	fetch_field_direct(nr)	*/
static VALUE fetch_field_direct(VALUE obj, VALUE nr)
{
    MYSQL_RES* res;
    unsigned int max;
    unsigned int n;
    check_free(obj);
    res = GetMysqlRes(obj);
    max = mysql_num_fields(res);
    n = NUM2INT(nr);
    if (n >= max)
#if RUBY_VERSION_CODE < 160
        Raise(eMysql, "%d: out of range (max: %d)", n, max-1);
#else
        rb_raise(eMysql, "%d: out of range (max: %d)", n, max-1);
#endif
#if MYSQL_VERSION_ID >= 32226
    return make_field_obj(mysql_fetch_field_direct(res, n));
#else
    return make_field_obj(&mysql_fetch_field_direct(res, n));
#endif
}

/*	fetch_lengths()		*/
static VALUE fetch_lengths(VALUE obj)
{
    MYSQL_RES* res;
    unsigned int n;
    unsigned long* lengths;
    VALUE ary;
    unsigned int i;
    check_free(obj);
    res = GetMysqlRes(obj);
    n = mysql_num_fields(res);
    lengths = mysql_fetch_lengths(res);
    if (lengths == NULL)
	return Qnil;
    ary = rb_ary_new2(n);
    for (i=0; i<n; i++)
	rb_ary_store(ary, i, INT2NUM(lengths[i]));
    return ary;
}

/*	fetch_row()	*/
static VALUE fetch_row(VALUE obj)
{
    MYSQL_RES* res;
    unsigned int n;
    MYSQL_ROW row;
    unsigned long* lengths;
    VALUE ary;
    unsigned int i;
    check_free(obj);
    res = GetMysqlRes(obj);
    n = mysql_num_fields(res);
    row = mysql_fetch_row(res);
    lengths = mysql_fetch_lengths(res);
    if (row == NULL)
	return Qnil;
    ary = rb_ary_new2(n);
    for (i=0; i<n; i++)
	rb_ary_store(ary, i, row[i]? rb_tainted_str_new(row[i], lengths[i]): Qnil);
    return ary;
}

/*	fetch_hash2 (internal)	*/
static VALUE fetch_hash2(VALUE obj, VALUE with_table)
{
    MYSQL_RES* res = GetMysqlRes(obj);
    unsigned int n = mysql_num_fields(res);
    MYSQL_ROW row = mysql_fetch_row(res);
    unsigned long* lengths = mysql_fetch_lengths(res);
    MYSQL_FIELD* fields = mysql_fetch_fields(res);
    unsigned int i;
    VALUE hash;
    if (row == NULL)
	return Qnil;
    hash = rb_hash_new();
    for (i=0; i<n; i++) {
	VALUE col;
	if (with_table == Qnil || with_table == Qfalse)
	    col = rb_tainted_str_new2(fields[i].name);
	else {
	    col = rb_tainted_str_new(fields[i].table, strlen(fields[i].table)+strlen(fields[i].name)+1);
	    RSTRING(col)->ptr[strlen(fields[i].table)] = '.';
	    strcpy(RSTRING(col)->ptr+strlen(fields[i].table)+1, fields[i].name);
	}
	rb_hash_aset(hash, col, row[i]? rb_tainted_str_new(row[i], lengths[i]): Qnil);
    }
    return hash;
}

/*	fetch_hash(with_table=false)	*/
static VALUE fetch_hash(int argc, VALUE* argv, VALUE obj)
{
    VALUE with_table;
    check_free(obj);
    rb_scan_args(argc, argv, "01", &with_table);
    if (with_table == Qnil)
	with_table = Qfalse;
    return fetch_hash2(obj, with_table);
}

/*	field_seek(offset)	*/
static VALUE field_seek(VALUE obj, VALUE offset)
{
    check_free(obj);
    return INT2NUM(mysql_field_seek(GetMysqlRes(obj), NUM2INT(offset)));
}

/*	field_tell()		*/
static VALUE field_tell(VALUE obj)
{
    check_free(obj);
    return INT2NUM(mysql_field_tell(GetMysqlRes(obj)));
}

/*	free()			*/
static VALUE res_free(VALUE obj)
{
    struct mysql_res* resp = DATA_PTR(obj);
    check_free(obj);
    mysql_free_result(resp->res);
    resp->freed = Qtrue;
    store_result_count--;
    return Qnil;
}

/*	num_fields()		*/
static VALUE num_fields(VALUE obj)
{
    check_free(obj);
    return INT2NUM(mysql_num_fields(GetMysqlRes(obj)));
}

/*	num_rows()	*/
static VALUE num_rows(VALUE obj)
{
    check_free(obj);
    return INT2NUM(mysql_num_rows(GetMysqlRes(obj)));
}

/*	row_seek(offset)	*/
static VALUE row_seek(VALUE obj, VALUE offset)
{
    MYSQL_ROW_OFFSET prev_offset;
    if (CLASS_OF(offset) != cMysqlRowOffset)
	rb_raise(rb_eTypeError, "wrong argument type %s (expected Mysql::RowOffset)", rb_obj_classname(offset));
    check_free(obj);
    prev_offset = mysql_row_seek(GetMysqlRes(obj), DATA_PTR(offset));
    return Data_Wrap_Struct(cMysqlRowOffset, 0, NULL, prev_offset);
}

/*	row_tell()	*/
static VALUE row_tell(VALUE obj)
{
    MYSQL_ROW_OFFSET offset;
    check_free(obj);
    offset = mysql_row_tell(GetMysqlRes(obj));
    return Data_Wrap_Struct(cMysqlRowOffset, 0, NULL, offset);
}

/*	each {...}	*/
static VALUE each(VALUE obj)
{
    VALUE row;
    check_free(obj);
    while ((row = fetch_row(obj)) != Qnil)
	rb_yield(row);
    return obj;
}

/*	each_hash(with_table=false) {...}	*/
static VALUE each_hash(int argc, VALUE* argv, VALUE obj)
{
    VALUE with_table;
    VALUE hash;
    check_free(obj);
    rb_scan_args(argc, argv, "01", &with_table);
    if (with_table == Qnil)
	with_table = Qfalse;
    while ((hash = fetch_hash2(obj, with_table)) != Qnil)
	rb_yield(hash);
    return obj;
}

/*-------------------------------
 * Mysql::Field object method
 */

/*	hash	*/
static VALUE field_hash(VALUE obj)
{
    VALUE h = rb_hash_new();
    rb_hash_aset(h, rb_str_new2("name"), rb_iv_get(obj, "name"));
    rb_hash_aset(h, rb_str_new2("table"), rb_iv_get(obj, "table"));
    rb_hash_aset(h, rb_str_new2("def"), rb_iv_get(obj, "def"));
    rb_hash_aset(h, rb_str_new2("type"), rb_iv_get(obj, "type"));
    rb_hash_aset(h, rb_str_new2("length"), rb_iv_get(obj, "length"));
    rb_hash_aset(h, rb_str_new2("max_length"), rb_iv_get(obj, "max_length"));
    rb_hash_aset(h, rb_str_new2("flags"), rb_iv_get(obj, "flags"));
    rb_hash_aset(h, rb_str_new2("decimals"), rb_iv_get(obj, "decimals"));
    return h;
}

/*	inspect	*/
static VALUE field_inspect(VALUE obj)
{
    VALUE n = rb_iv_get(obj, "name");
    VALUE s = rb_str_new(0, RSTRING(n)->len + 16);
    sprintf(RSTRING(s)->ptr, "#<Mysql::Field:%s>", RSTRING(n)->ptr);
    return s;
}

#define DefineMysqlFieldMemberMethod(m)\
static VALUE field_##m(VALUE obj)\
{return rb_iv_get(obj, #m);}

DefineMysqlFieldMemberMethod(name)
DefineMysqlFieldMemberMethod(table)
DefineMysqlFieldMemberMethod(def)
DefineMysqlFieldMemberMethod(type)
DefineMysqlFieldMemberMethod(length)
DefineMysqlFieldMemberMethod(max_length)
DefineMysqlFieldMemberMethod(flags)
DefineMysqlFieldMemberMethod(decimals)

#ifdef IS_NUM
/*	is_num?	*/
static VALUE field_is_num(VALUE obj)
{
    return IS_NUM(NUM2INT(rb_iv_get(obj, "type"))) ? Qtrue : Qfalse;
}
#endif

#ifdef IS_NOT_NULL
/*	is_not_null?	*/
static VALUE field_is_not_null(VALUE obj)
{
    return IS_NOT_NULL(NUM2INT(rb_iv_get(obj, "flags"))) ? Qtrue : Qfalse;
}
#endif

#ifdef IS_PRI_KEY
/*	is_pri_key?	*/
static VALUE field_is_pri_key(VALUE obj)
{
    return IS_PRI_KEY(NUM2INT(rb_iv_get(obj, "flags"))) ? Qtrue : Qfalse;
}
#endif

#if MYSQL_VERSION_ID >= 40102
/*-------------------------------
 * Mysql::Stmt object method
 */

/*	check if stmt is already closed */
static void check_stmt_closed(VALUE obj)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    if (s->closed == Qtrue)
	rb_raise(eMysql, "Mysql::Stmt object is already closed");
}

static void mysql_stmt_raise(MYSQL_STMT* s)
{
    VALUE e = rb_exc_new2(eMysql, mysql_stmt_error(s));
    rb_iv_set(e, "errno", INT2FIX(mysql_stmt_errno(s)));
    rb_iv_set(e, "sqlstate", rb_tainted_str_new2(mysql_stmt_sqlstate(s)));
    rb_exc_raise(e);
}

/*	affected_rows()	*/
static VALUE stmt_affected_rows(VALUE obj)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    my_ulonglong n;
    check_stmt_closed(obj);
    n = mysql_stmt_affected_rows(s->stmt);
    return INT2NUM(n);
}

#if 0
/*	attr_get(option)	*/
static VALUE stmt_attr_get(VALUE obj, VALUE opt)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    check_stmt_closed(obj);
    if (NUM2INT(opt) == STMT_ATTR_UPDATE_MAX_LENGTH) {
	my_bool arg;
	mysql_stmt_attr_get(s->stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &arg);
	return arg == 1 ? Qtrue : Qfalse;
    }
    rb_raise(eMysql, "unknown option: %d", NUM2INT(opt));
}

/*	attr_set(option, arg)	*/
static VALUE stmt_attr_set(VALUE obj, VALUE opt, VALUE val)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    check_stmt_closed(obj);
    if (NUM2INT(opt) == STMT_ATTR_UPDATE_MAX_LENGTH) {
	my_bool arg;
	arg = (val == Qnil || val == Qfalse) ? 0 : 1;
	mysql_stmt_attr_set(s->stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &arg);
	return obj;
    }
    rb_raise(eMysql, "unknown option: %d", NUM2INT(opt));
}
#endif

/*	bind_result(bind,...)	*/
static enum enum_field_types buffer_type(MYSQL_FIELD *field);
static VALUE stmt_bind_result(int argc, VALUE *argv, VALUE obj)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    int i;
    MYSQL_FIELD *field;

    check_stmt_closed(obj);
    if (argc != s->result.n)
	rb_raise(eMysql, "bind_result: result value count(%d) != number of argument(%d)", s->result.n, argc);
    for (i = 0; i < argc; i++) {
	if (argv[i] == Qnil || argv[i] == rb_cNilClass) {
	    field = mysql_fetch_fields(s->res);
	    s->result.bind[i].buffer_type = buffer_type(&field[i]);
	}
	else if (argv[i] == rb_cString)
	    s->result.bind[i].buffer_type = MYSQL_TYPE_STRING;
	else if (argv[i] == rb_cNumeric || argv[i] == rb_cInteger || argv[i] == rb_cFixnum)
	    s->result.bind[i].buffer_type = MYSQL_TYPE_LONGLONG;
	else if (argv[i] == rb_cFloat)
	    s->result.bind[i].buffer_type = MYSQL_TYPE_DOUBLE;
	else if (argv[i] == cMysqlTime)
	    s->result.bind[i].buffer_type = MYSQL_TYPE_DATETIME;
	else
	    rb_raise(rb_eTypeError, "unrecognized class: %s", RSTRING(rb_inspect(argv[i]))->ptr);
	if (mysql_stmt_bind_result(s->stmt, s->result.bind))
	    mysql_stmt_raise(s->stmt);
    }
    return obj;
}

/*	close()	*/
static VALUE stmt_close(VALUE obj)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    check_stmt_closed(obj);
    mysql_stmt_close(s->stmt);
    s->closed = Qtrue;
    return Qnil;
}

/*	data_seek(offset)	*/
static VALUE stmt_data_seek(VALUE obj, VALUE offset)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    check_stmt_closed(obj);
    mysql_stmt_data_seek(s->stmt, NUM2INT(offset));
    return obj;
}

/*	execute(arg,...)	*/
static VALUE stmt_execute(int argc, VALUE *argv, VALUE obj)
{
    struct mysql_stmt *s = DATA_PTR(obj);
    MYSQL_STMT *stmt = s->stmt;
    my_bool true = 1;
    my_bool false = 0;
    int i;

    check_stmt_closed(obj);
    if (s->param.n != argc)
	rb_raise(eMysql, "execute: param_count(%d) != number of argument(%d)", s->param.n, argc);
    memset(s->param.bind, 0, sizeof(*(s->param.bind))*argc);
    for (i = 0; i < argc; i++) {
	switch (TYPE(argv[i])) {
	case T_NIL:
	    s->param.bind[i].buffer_type = MYSQL_TYPE_NULL;
	    s->param.bind[i].is_null = &true;
	    break;
	case T_FIXNUM:
	    s->param.bind[i].buffer_type = MYSQL_TYPE_LONG;
	    s->param.bind[i].buffer = &(s->param.buffer[i]);
	    *(long*)(s->param.bind[i].buffer) = FIX2INT(argv[i]);
	    break;
	case T_BIGNUM:
	    s->param.bind[i].buffer_type = MYSQL_TYPE_LONGLONG;
	    s->param.bind[i].buffer = &(s->param.buffer[i]);
	    *(long long*)(s->param.bind[i].buffer) = rb_big2ll(argv[i]);
	    break;
	case T_FLOAT:
	    s->param.bind[i].buffer_type = MYSQL_TYPE_DOUBLE;
	    s->param.bind[i].buffer = &(s->param.buffer[i]);
	    *(double*)(s->param.bind[i].buffer) = NUM2DBL(argv[i]);
	    break;
	case T_STRING:
	    s->param.bind[i].buffer_type = MYSQL_TYPE_STRING;
	    s->param.bind[i].buffer = RSTRING(argv[i])->ptr;
	    s->param.bind[i].buffer_length = RSTRING(argv[i])->len;
	    s->param.length[i] = RSTRING(argv[i])->len;
	    s->param.bind[i].length = &(s->param.length[i]);
	    break;
	default:
	    if (CLASS_OF(argv[i]) == rb_cTime) {
		MYSQL_TIME t;
		VALUE a = rb_funcall(argv[i], rb_intern("to_a"), 0);
		s->param.bind[i].buffer_type = MYSQL_TYPE_DATETIME;
		s->param.bind[i].buffer = &(s->param.buffer[i]);
		t.second_part = 0;
		t.neg = 0;
		t.second = FIX2INT(RARRAY(a)->ptr[0]);
		t.minute = FIX2INT(RARRAY(a)->ptr[1]);
		t.hour = FIX2INT(RARRAY(a)->ptr[2]);
		t.day = FIX2INT(RARRAY(a)->ptr[3]);
		t.month = FIX2INT(RARRAY(a)->ptr[4]);
		t.year = FIX2INT(RARRAY(a)->ptr[5]);
		*(MYSQL_TIME*)&(s->param.buffer[i]) = t;
	    } else if (CLASS_OF(argv[i]) == cMysqlTime) {
		MYSQL_TIME t;
		s->param.bind[i].buffer_type = MYSQL_TYPE_DATETIME;
		s->param.bind[i].buffer = &(s->param.buffer[i]);
		t.second_part = 0;
		t.neg = 0;
		t.second = NUM2INT(rb_iv_get(argv[i], "second"));
		t.minute = NUM2INT(rb_iv_get(argv[i], "minute"));
		t.hour = NUM2INT(rb_iv_get(argv[i], "hour"));
		t.day = NUM2INT(rb_iv_get(argv[i], "day"));
		t.month = NUM2INT(rb_iv_get(argv[i], "month"));
		t.year = NUM2INT(rb_iv_get(argv[i], "year"));
		*(MYSQL_TIME*)&(s->param.buffer[i]) = t;
	    } else
		rb_raise(rb_eTypeError, "unsupported type: %d", TYPE(argv[i]));
	}
    }
    if (mysql_stmt_bind_param(stmt, s->param.bind))
	mysql_stmt_raise(stmt);

    if (mysql_stmt_execute(stmt))
	mysql_stmt_raise(stmt);
    if (s->res) {
	MYSQL_FIELD *field;
	if (mysql_stmt_store_result(stmt))
	    mysql_stmt_raise(stmt);
	field = mysql_fetch_fields(s->res);
	for (i = 0; i < s->result.n; i++) {
	    if (s->result.bind[i].buffer_type == MYSQL_TYPE_STRING ||
		s->result.bind[i].buffer_type == MYSQL_TYPE_BLOB) {
		s->result.bind[i].buffer = xmalloc(field[i].max_length);
                memset(s->result.bind[i].buffer, 0, field[i].max_length);
		s->result.bind[i].buffer_length = field[i].max_length;
	    } else {
		s->result.bind[i].buffer = xmalloc(sizeof(MYSQL_TIME));
		s->result.bind[i].buffer_length = sizeof(MYSQL_TIME);
                memset(s->result.bind[i].buffer, 0, sizeof(MYSQL_TIME));
	    }
	}
	if (mysql_stmt_bind_result(s->stmt, s->result.bind))
	    mysql_stmt_raise(s->stmt);
    }
    return obj;
}

/*	fetch()		*/
static VALUE stmt_fetch(VALUE obj)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    VALUE ret;
    int i;
    int r;

    check_stmt_closed(obj);
    r = mysql_stmt_fetch(s->stmt);
    if (r == MYSQL_NO_DATA)
	return Qnil;
    if (r == 1)
	mysql_stmt_raise(s->stmt);

    ret = rb_ary_new2(s->result.n);
    for (i = 0; i < s->result.n; i++) {
	if (s->result.is_null[i])
	    rb_ary_push(ret, Qnil);
	else {
	    VALUE v;
	    MYSQL_TIME *t;
	    switch (s->result.bind[i].buffer_type) {
	    case MYSQL_TYPE_LONG:
		v = INT2NUM(*(long*)s->result.bind[i].buffer);
		break;
	    case MYSQL_TYPE_LONGLONG:
		v = rb_ll2inum(*(long long*)s->result.bind[i].buffer);
		break;
	    case MYSQL_TYPE_DOUBLE:
		v = rb_float_new(*(double*)s->result.bind[i].buffer);
		break;
	    case MYSQL_TYPE_TIMESTAMP:
	    case MYSQL_TYPE_DATE:
	    case MYSQL_TYPE_TIME:
	    case MYSQL_TYPE_DATETIME:
		t = (MYSQL_TIME*)s->result.bind[i].buffer;
		v = rb_obj_alloc(cMysqlTime);
		rb_funcall(v, rb_intern("initialize"), 8,
			   INT2FIX(t->year), INT2FIX(t->month),
			   INT2FIX(t->day), INT2FIX(t->hour),
			   INT2FIX(t->minute), INT2FIX(t->second),
			   (t->neg ? Qtrue : Qfalse),
			   INT2FIX(t->second_part));
		break;
	    case MYSQL_TYPE_STRING:
	    case MYSQL_TYPE_BLOB:
		v = rb_tainted_str_new(s->result.bind[i].buffer, s->result.length[i]);
		break;
	    default:
		rb_raise(rb_eTypeError, "unknown buffer_type: %d", s->result.bind[i].buffer_type);
	    }
	    rb_ary_push(ret, v);
	}
    }
    return ret;
}

/*	each {...}	*/
static VALUE stmt_each(VALUE obj)
{
    VALUE row;
    check_stmt_closed(obj);
    while ((row = stmt_fetch(obj)) != Qnil)
	rb_yield(row);
    return obj;
}

/*	field_count()	*/
static VALUE stmt_field_count(VALUE obj)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    unsigned int n;
    check_stmt_closed(obj);
    n = mysql_stmt_field_count(s->stmt);
    return INT2NUM(n);
}

/*	free_result()	*/
static VALUE stmt_free_result(VALUE obj)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    check_stmt_closed(obj);
    if (s->res) {
	mysql_free_result(s->res);
	s->res = NULL;
    }
    if (mysql_stmt_free_result(s->stmt))
	mysql_stmt_raise(s->stmt);
    return obj;
}

/*	insert_id()	*/
static VALUE stmt_insert_id(VALUE obj)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    my_ulonglong n;
    check_stmt_closed(obj);
    n = mysql_stmt_insert_id(s->stmt);
    return INT2NUM(n);
}

/*	num_rows()	*/
static VALUE stmt_num_rows(VALUE obj)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    my_ulonglong n;
    check_stmt_closed(obj);
    n = mysql_stmt_num_rows(s->stmt);
    return INT2NUM(n);
}

/*	param_count()		*/
static VALUE stmt_param_count(VALUE obj)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    unsigned long n;
    check_stmt_closed(obj);
    n = mysql_stmt_param_count(s->stmt);
    return INT2NUM(n);
}

/*	prepare(query)	*/
static enum enum_field_types buffer_type(MYSQL_FIELD *field)
{
    switch (field->type) {
    case FIELD_TYPE_TINY:
    case FIELD_TYPE_SHORT:
    case FIELD_TYPE_INT24:
    case FIELD_TYPE_YEAR:
	return MYSQL_TYPE_LONG;
    case FIELD_TYPE_LONG:
    case FIELD_TYPE_LONGLONG:
	return MYSQL_TYPE_LONGLONG;
    case FIELD_TYPE_FLOAT:
    case FIELD_TYPE_DOUBLE:
	return MYSQL_TYPE_DOUBLE;
    case FIELD_TYPE_TIMESTAMP:
	return MYSQL_TYPE_TIMESTAMP;
    case FIELD_TYPE_DATE:
	return MYSQL_TYPE_DATE;
    case FIELD_TYPE_TIME:
	return MYSQL_TYPE_TIME;
    case FIELD_TYPE_DATETIME:
	return MYSQL_TYPE_DATETIME;
    case FIELD_TYPE_STRING:
    case FIELD_TYPE_VAR_STRING:
    case FIELD_TYPE_SET:
    case FIELD_TYPE_ENUM:
    case FIELD_TYPE_DECIMAL:
#if MYSQL_VERSION_ID >= 50003
    case FIELD_TYPE_NEWDECIMAL:
#endif
	return MYSQL_TYPE_STRING;
    case FIELD_TYPE_BLOB:
	return MYSQL_TYPE_BLOB;
    case FIELD_TYPE_NULL:
	return MYSQL_TYPE_NULL;
    default:
	rb_raise(rb_eTypeError, "unknown type: %d", field->type);
    }
}

static VALUE stmt_prepare(VALUE obj, VALUE query)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    int n;
    int i;
    MYSQL_FIELD *field;

    free_mysqlstmt_memory(s);
    check_stmt_closed(obj);
    Check_Type(query, T_STRING);
    if (mysql_stmt_prepare(s->stmt, RSTRING(query)->ptr, RSTRING(query)->len))
	mysql_stmt_raise(s->stmt);

    n = mysql_stmt_param_count(s->stmt);
    s->param.n = n;
    s->param.bind = xmalloc(sizeof(s->param.bind[0]) * n);
    s->param.length = xmalloc(sizeof(s->param.length[0]) * n);
    s->param.buffer = xmalloc(sizeof(s->param.buffer[0]) * n);

    s->res = mysql_stmt_result_metadata(s->stmt);
    if (s->res) {
	n = s->result.n = mysql_num_fields(s->res);
	s->result.bind = xmalloc(sizeof(s->result.bind[0]) * n);
	s->result.is_null = xmalloc(sizeof(s->result.is_null[0]) * n);
	s->result.length = xmalloc(sizeof(s->result.length[0]) * n);
	field = mysql_fetch_fields(s->res);
	memset(s->result.bind, 0, sizeof(s->result.bind[0]) * n);
	for (i = 0; i < n; i++) {
	    s->result.bind[i].buffer_type = buffer_type(&field[i]);
	    s->result.bind[i].is_null = &(s->result.is_null[i]);
	    s->result.bind[i].length = &(s->result.length[i]);
	}
    } else {
	if (mysql_stmt_errno(s->stmt))
	    mysql_stmt_raise(s->stmt);
    }

    return obj;
}

#if 0
/*	reset()		*/
static VALUE stmt_reset(VALUE obj)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    check_stmt_closed(obj);
    if (mysql_stmt_reset(s->stmt))
	mysql_stmt_raise(s->stmt);
    return obj;
}
#endif

/*	result_metadata()	*/
static VALUE stmt_result_metadata(VALUE obj)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    MYSQL_RES *res;
    check_stmt_closed(obj);
    res = mysql_stmt_result_metadata(s->stmt);
    if (res == NULL && mysql_stmt_errno(s->stmt) != 0)
	mysql_stmt_raise(s->stmt);
    return mysqlres2obj(res);
}

/*	row_seek(offset)	*/
static VALUE stmt_row_seek(VALUE obj, VALUE offset)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    MYSQL_ROW_OFFSET prev_offset;
    if (CLASS_OF(offset) != cMysqlRowOffset)
	rb_raise(rb_eTypeError, "wrong argument type %s (expected Mysql::RowOffset)", rb_obj_classname(offset));
    check_stmt_closed(obj);
    prev_offset = mysql_stmt_row_seek(s->stmt, DATA_PTR(offset));
    return Data_Wrap_Struct(cMysqlRowOffset, 0, NULL, prev_offset);
}

/*	row_tell()	*/
static VALUE stmt_row_tell(VALUE obj)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    MYSQL_ROW_OFFSET offset;
    check_stmt_closed(obj);
    offset = mysql_stmt_row_tell(s->stmt);
    return Data_Wrap_Struct(cMysqlRowOffset, 0, NULL, offset);
}

#if 0
/*	send_long_data(col, data)	*/
static VALUE stmt_send_long_data(VALUE obj, VALUE col, VALUE data)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    int c;
    check_stmt_closed(obj);
    c = NUM2INT(col);
    if (0 <= c && c < s->param.n) {
	s->param.bind[c].buffer_type = MYSQL_TYPE_STRING;
	if (mysql_stmt_bind_param(s->stmt, s->param.bind))
	    mysql_stmt_raise(s->stmt);
    }
    if (mysql_stmt_send_long_data(s->stmt, c, RSTRING(data)->ptr, RSTRING(data)->len))
	mysql_stmt_raise(s->stmt);
    return obj;
}
#endif

/*	sqlstate()	*/
static VALUE stmt_sqlstate(VALUE obj)
{
    struct mysql_stmt* s = DATA_PTR(obj);
    return rb_tainted_str_new2(mysql_stmt_sqlstate(s->stmt));
}

/*-------------------------------
 * Mysql::Time object method
 */

static VALUE time_initialize(int argc, VALUE* argv, VALUE obj)
{
    VALUE year, month, day, hour, minute, second, neg, second_part;
    rb_scan_args(argc, argv, "08", &year, &month, &day, &hour, &minute, &second, &neg, &second_part);
#define NILorFIXvalue(o)	(NIL_P(o) ? INT2FIX(0) : (Check_Type(o, T_FIXNUM), o))
    rb_iv_set(obj, "year", NILorFIXvalue(year));
    rb_iv_set(obj, "month", NILorFIXvalue(month));
    rb_iv_set(obj, "day", NILorFIXvalue(day));
    rb_iv_set(obj, "hour", NILorFIXvalue(hour));
    rb_iv_set(obj, "minute", NILorFIXvalue(minute));
    rb_iv_set(obj, "second", NILorFIXvalue(second));
    rb_iv_set(obj, "neg", (neg == Qnil || neg == Qfalse) ? Qfalse : Qtrue);
    rb_iv_set(obj, "second_part", NILorFIXvalue(second_part));
}

static VALUE time_inspect(VALUE obj)
{
    char buf[36];
    sprintf(buf, "#<Mysql::Time:%04d-%02d-%02d %02d:%02d:%02d>",
	    NUM2INT(rb_iv_get(obj, "year")),
	    NUM2INT(rb_iv_get(obj, "month")),
	    NUM2INT(rb_iv_get(obj, "day")),
	    NUM2INT(rb_iv_get(obj, "hour")),
	    NUM2INT(rb_iv_get(obj, "minute")),
	    NUM2INT(rb_iv_get(obj, "second")));
    return rb_str_new2(buf);
}

static VALUE time_to_s(VALUE obj)
{
    char buf[20];
    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
	    NUM2INT(rb_iv_get(obj, "year")),
	    NUM2INT(rb_iv_get(obj, "month")),
	    NUM2INT(rb_iv_get(obj, "day")),
	    NUM2INT(rb_iv_get(obj, "hour")),
	    NUM2INT(rb_iv_get(obj, "minute")),
	    NUM2INT(rb_iv_get(obj, "second")));
    return rb_str_new2(buf);
}

#define DefineMysqlTimeGetMethod(m)\
static VALUE time_get_##m(VALUE obj)\
{return rb_iv_get(obj, #m);}

DefineMysqlTimeGetMethod(year)
DefineMysqlTimeGetMethod(month)
DefineMysqlTimeGetMethod(day)
DefineMysqlTimeGetMethod(hour)
DefineMysqlTimeGetMethod(minute)
DefineMysqlTimeGetMethod(second)
DefineMysqlTimeGetMethod(neg)
DefineMysqlTimeGetMethod(second_part)

#define DefineMysqlTimeSetMethod(m)\
static VALUE time_set_##m(VALUE obj, VALUE v)\
{rb_iv_set(obj, #m, NILorFIXvalue(v)); return v;}

DefineMysqlTimeSetMethod(year)
DefineMysqlTimeSetMethod(month)
DefineMysqlTimeSetMethod(day)
DefineMysqlTimeSetMethod(hour)
DefineMysqlTimeSetMethod(minute)
DefineMysqlTimeSetMethod(second)
DefineMysqlTimeSetMethod(second_part)

static VALUE time_set_neg(VALUE obj, VALUE v)
{
    rb_iv_set(obj, "neg", (v == Qnil || v == Qfalse) ? Qfalse : Qtrue);
    return v;
}

static VALUE time_equal(VALUE obj, VALUE v)
{
    if (CLASS_OF(v) == cMysqlTime &&
	NUM2INT(rb_iv_get(obj, "year")) == NUM2INT(rb_iv_get(v, "year")) &&
	NUM2INT(rb_iv_get(obj, "month")) == NUM2INT(rb_iv_get(v, "month")) &&
	NUM2INT(rb_iv_get(obj, "day")) == NUM2INT(rb_iv_get(v, "day")) &&
	NUM2INT(rb_iv_get(obj, "hour")) == NUM2INT(rb_iv_get(v, "hour")) &&
	NUM2INT(rb_iv_get(obj, "minute")) == NUM2INT(rb_iv_get(v, "minute")) &&
	NUM2INT(rb_iv_get(obj, "second")) == NUM2INT(rb_iv_get(v, "second")) &&
	rb_iv_get(obj, "neg") == rb_iv_get(v, "neg") &&
	NUM2INT(rb_iv_get(obj, "second_part")) == NUM2INT(rb_iv_get(v, "second_part")))
	return Qtrue;
    return Qfalse;
}

#endif

/*-------------------------------
 * Mysql::Error object method
 */

static VALUE error_error(VALUE obj)
{
    return rb_iv_get(obj, "mesg");
}

static VALUE error_errno(VALUE obj)
{
    return rb_iv_get(obj, "errno");
}

static VALUE error_sqlstate(VALUE obj)
{
    return rb_iv_get(obj, "sqlstate");
}

/*-------------------------------
 *	Initialize
 */

void Init_mysql(void)
{
    cMysql = rb_define_class("Mysql", rb_cObject);
    cMysqlRes = rb_define_class_under(cMysql, "Result", rb_cObject);
    cMysqlField = rb_define_class_under(cMysql, "Field", rb_cObject);
#if MYSQL_VERSION_ID >= 40102
    cMysqlStmt = rb_define_class_under(cMysql, "Stmt", rb_cObject);
    cMysqlRowOffset = rb_define_class_under(cMysql, "RowOffset", rb_cObject);
    cMysqlTime = rb_define_class_under(cMysql, "Time", rb_cObject);
#endif
    eMysql = rb_define_class_under(cMysql, "Error", rb_eStandardError);

    rb_define_global_const("MysqlRes", cMysqlRes);
    rb_define_global_const("MysqlField", cMysqlField);
    rb_define_global_const("MysqlError", eMysql);

    /* Mysql class method */
    rb_define_singleton_method(cMysql, "init", init, 0);
    rb_define_singleton_method(cMysql, "real_connect", real_connect, -1);
    rb_define_singleton_method(cMysql, "connect", real_connect, -1);
    rb_define_singleton_method(cMysql, "new", real_connect, -1);
    rb_define_singleton_method(cMysql, "escape_string", escape_string, 1);
    rb_define_singleton_method(cMysql, "quote", escape_string, 1);
    rb_define_singleton_method(cMysql, "client_info", client_info, 0);
    rb_define_singleton_method(cMysql, "get_client_info", client_info, 0);
#if MYSQL_VERSION_ID >= 32332
    rb_define_singleton_method(cMysql, "debug", my_debug, 1);
#endif
#if MYSQL_VERSION_ID >= 40000
    rb_define_singleton_method(cMysql, "get_client_version", client_version, 0);
    rb_define_singleton_method(cMysql, "client_version", client_version, 0);
#endif

    /* Mysql object method */
#if MYSQL_VERSION_ID >= 32200
    rb_define_method(cMysql, "real_connect", real_connect2, -1);
    rb_define_method(cMysql, "connect", real_connect2, -1);
    rb_define_method(cMysql, "options", options, -1);
#endif
    rb_define_method(cMysql, "initialize", initialize, -1);
#if MYSQL_VERSION_ID >= 32332
    rb_define_method(cMysql, "escape_string", real_escape_string, 1);
    rb_define_method(cMysql, "quote", real_escape_string, 1);
#else
    rb_define_method(cMysql, "escape_string", escape_string, 1);
    rb_define_method(cMysql, "quote", escape_string, 1);
#endif
    rb_define_method(cMysql, "client_info", client_info, 0);
    rb_define_method(cMysql, "get_client_info", client_info, 0);
    rb_define_method(cMysql, "affected_rows", affected_rows, 0);
#if MYSQL_VERSION_ID >= 32303
    rb_define_method(cMysql, "change_user", change_user, -1);
#endif
#if MYSQL_VERSION_ID >= 32321
    rb_define_method(cMysql, "character_set_name", character_set_name, 0);
#endif
    rb_define_method(cMysql, "close", my_close, 0);
#if MYSQL_VERSION_ID < 40000
    rb_define_method(cMysql, "create_db", create_db, 1);
    rb_define_method(cMysql, "drop_db", drop_db, 1);
#endif
#if MYSQL_VERSION_ID >= 32332
    rb_define_method(cMysql, "dump_debug_info", dump_debug_info, 0);
#endif
    rb_define_method(cMysql, "errno", my_errno, 0);
    rb_define_method(cMysql, "error", my_error, 0);
    rb_define_method(cMysql, "field_count", field_count, 0);
#if MYSQL_VERSION_ID >= 40000
    rb_define_method(cMysql, "get_client_version", client_version, 0);
    rb_define_method(cMysql, "client_version", client_version, 0);
#endif
    rb_define_method(cMysql, "get_host_info", host_info, 0);
    rb_define_method(cMysql, "host_info", host_info, 0);
    rb_define_method(cMysql, "get_proto_info", proto_info, 0);
    rb_define_method(cMysql, "proto_info", proto_info, 0);
    rb_define_method(cMysql, "get_server_info", server_info, 0);
    rb_define_method(cMysql, "server_info", server_info, 0);
    rb_define_method(cMysql, "info", info, 0);
    rb_define_method(cMysql, "insert_id", insert_id, 0);
    rb_define_method(cMysql, "kill", my_kill, 1);
    rb_define_method(cMysql, "list_dbs", list_dbs, -1);
    rb_define_method(cMysql, "list_fields", list_fields, -1);
    rb_define_method(cMysql, "list_processes", list_processes, 0);
    rb_define_method(cMysql, "list_tables", list_tables, -1);
#if MYSQL_VERSION_ID >= 32200
    rb_define_method(cMysql, "ping", ping, 0);
#endif
    rb_define_method(cMysql, "query", query, 1);
    rb_define_method(cMysql, "real_query", query, 1);
    rb_define_method(cMysql, "refresh", refresh, 1);
    rb_define_method(cMysql, "reload", reload, 0);
    rb_define_method(cMysql, "select_db", select_db, 1);
    rb_define_method(cMysql, "shutdown", my_shutdown, -1);
    rb_define_method(cMysql, "stat", my_stat, 0);
    rb_define_method(cMysql, "store_result", store_result, 0);
    rb_define_method(cMysql, "thread_id", thread_id, 0);
    rb_define_method(cMysql, "use_result", use_result, 0);
#if MYSQL_VERSION_ID >= 40100
    rb_define_method(cMysql, "get_server_version", server_version, 0);
    rb_define_method(cMysql, "server_version", server_version, 0);
    rb_define_method(cMysql, "warning_count", warning_count, 0);
    rb_define_method(cMysql, "commit", commit, 0);
    rb_define_method(cMysql, "rollback", rollback, 0);
    rb_define_method(cMysql, "autocommit", autocommit, 1);
#endif
#ifdef HAVE_MYSQL_SSL_SET
    rb_define_method(cMysql, "ssl_set", ssl_set, -1);
#endif
#if MYSQL_VERSION_ID >= 40102
    rb_define_method(cMysql, "stmt_init", stmt_init, 0);
    rb_define_method(cMysql, "prepare", prepare, 1);
#endif
#if MYSQL_VERSION_ID >= 40100
    rb_define_method(cMysql, "more_results", more_results, 0);
    rb_define_method(cMysql, "more_results?", more_results, 0);
    rb_define_method(cMysql, "next_result", next_result, 0);
#endif
#if MYSQL_VERSION_ID >= 40101
    rb_define_method(cMysql, "set_server_option", set_server_option, 1);
    rb_define_method(cMysql, "sqlstate", sqlstate, 0);
#endif
    rb_define_method(cMysql, "query_with_result", query_with_result, 0);
    rb_define_method(cMysql, "query_with_result=", query_with_result_set, 1);

    rb_define_method(cMysql, "reconnect", reconnect, 0);
    rb_define_method(cMysql, "reconnect=", reconnect_set, 1);

    /* Mysql constant */
    rb_define_const(cMysql, "VERSION", INT2FIX(MYSQL_RUBY_VERSION));
#if MYSQL_VERSION_ID >= 32200
    rb_define_const(cMysql, "OPT_CONNECT_TIMEOUT", INT2NUM(MYSQL_OPT_CONNECT_TIMEOUT));
    rb_define_const(cMysql, "OPT_COMPRESS", INT2NUM(MYSQL_OPT_COMPRESS));
    rb_define_const(cMysql, "OPT_NAMED_PIPE", INT2NUM(MYSQL_OPT_NAMED_PIPE));
    rb_define_const(cMysql, "INIT_COMMAND", INT2NUM(MYSQL_INIT_COMMAND));
    rb_define_const(cMysql, "READ_DEFAULT_FILE", INT2NUM(MYSQL_READ_DEFAULT_FILE));
    rb_define_const(cMysql, "READ_DEFAULT_GROUP", INT2NUM(MYSQL_READ_DEFAULT_GROUP));
#endif
#if MYSQL_VERSION_ID >= 32349
    rb_define_const(cMysql, "SET_CHARSET_DIR", INT2NUM(MYSQL_SET_CHARSET_DIR));
    rb_define_const(cMysql, "SET_CHARSET_NAME", INT2NUM(MYSQL_SET_CHARSET_NAME));
    rb_define_const(cMysql, "OPT_LOCAL_INFILE", INT2NUM(MYSQL_OPT_LOCAL_INFILE));
#endif
#if MYSQL_VERSION_ID >= 40100
    rb_define_const(cMysql, "OPT_PROTOCOL", INT2NUM(MYSQL_OPT_PROTOCOL));
    rb_define_const(cMysql, "SHARED_MEMORY_BASE_NAME", INT2NUM(MYSQL_SHARED_MEMORY_BASE_NAME));
#endif
#if MYSQL_VERSION_ID >= 40101
    rb_define_const(cMysql, "OPT_READ_TIMEOUT", INT2NUM(MYSQL_OPT_READ_TIMEOUT));
    rb_define_const(cMysql, "OPT_WRITE_TIMEOUT", INT2NUM(MYSQL_OPT_WRITE_TIMEOUT));
    rb_define_const(cMysql, "SECURE_AUTH", INT2NUM(MYSQL_SECURE_AUTH));
    rb_define_const(cMysql, "OPT_GUESS_CONNECTION", INT2NUM(MYSQL_OPT_GUESS_CONNECTION));
    rb_define_const(cMysql, "OPT_USE_EMBEDDED_CONNECTION", INT2NUM(MYSQL_OPT_USE_EMBEDDED_CONNECTION));
    rb_define_const(cMysql, "OPT_USE_REMOTE_CONNECTION", INT2NUM(MYSQL_OPT_USE_REMOTE_CONNECTION));
    rb_define_const(cMysql, "SET_CLIENT_IP", INT2NUM(MYSQL_SET_CLIENT_IP));
#endif
    rb_define_const(cMysql, "REFRESH_GRANT", INT2NUM(REFRESH_GRANT));
    rb_define_const(cMysql, "REFRESH_LOG", INT2NUM(REFRESH_LOG));
    rb_define_const(cMysql, "REFRESH_TABLES", INT2NUM(REFRESH_TABLES));
#ifdef REFRESH_HOSTS
    rb_define_const(cMysql, "REFRESH_HOSTS", INT2NUM(REFRESH_HOSTS));
#endif
#ifdef REFRESH_STATUS
    rb_define_const(cMysql, "REFRESH_STATUS", INT2NUM(REFRESH_STATUS));
#endif
#ifdef REFRESH_THREADS
    rb_define_const(cMysql, "REFRESH_THREADS", INT2NUM(REFRESH_THREADS));
#endif
#ifdef REFRESH_SLAVE
    rb_define_const(cMysql, "REFRESH_SLAVE", INT2NUM(REFRESH_SLAVE));
#endif
#ifdef REFRESH_MASTER
    rb_define_const(cMysql, "REFRESH_MASTER", INT2NUM(REFRESH_MASTER));
#endif
#ifdef CLIENT_LONG_PASSWORD
#endif
#ifdef CLIENT_FOUND_ROWS
    rb_define_const(cMysql, "CLIENT_FOUND_ROWS", INT2NUM(CLIENT_FOUND_ROWS));
#endif
#ifdef CLIENT_LONG_FLAG
#endif
#ifdef CLIENT_CONNECT_WITH_DB
#endif
#ifdef CLIENT_NO_SCHEMA
    rb_define_const(cMysql, "CLIENT_NO_SCHEMA", INT2NUM(CLIENT_NO_SCHEMA));
#endif
#ifdef CLIENT_COMPRESS
    rb_define_const(cMysql, "CLIENT_COMPRESS", INT2NUM(CLIENT_COMPRESS));
#endif
#ifdef CLIENT_ODBC
    rb_define_const(cMysql, "CLIENT_ODBC", INT2NUM(CLIENT_ODBC));
#endif
#ifdef CLIENT_LOCAL_FILES
    rb_define_const(cMysql, "CLIENT_LOCAL_FILES", INT2NUM(CLIENT_LOCAL_FILES));
#endif
#ifdef CLIENT_IGNORE_SPACE
    rb_define_const(cMysql, "CLIENT_IGNORE_SPACE", INT2NUM(CLIENT_IGNORE_SPACE));
#endif
#ifdef CLIENT_CHANGE_USER
    rb_define_const(cMysql, "CLIENT_CHANGE_USER", INT2NUM(CLIENT_CHANGE_USER));
#endif
#ifdef CLIENT_INTERACTIVE
    rb_define_const(cMysql, "CLIENT_INTERACTIVE", INT2NUM(CLIENT_INTERACTIVE));
#endif
#ifdef CLIENT_SSL
    rb_define_const(cMysql, "CLIENT_SSL", INT2NUM(CLIENT_SSL));
#endif
#ifdef CLIENT_IGNORE_SIGPIPE
    rb_define_const(cMysql, "CLIENT_IGNORE_SIGPIPE", INT2NUM(CLIENT_IGNORE_SIGPIPE));
#endif
#ifdef CLIENT_TRANSACTIONS
    rb_define_const(cMysql, "CLIENT_TRANSACTIONS", INT2NUM(CLIENT_TRANSACTIONS));
#endif
#ifdef CLIENT_MULTI_STATEMENTS
    rb_define_const(cMysql, "CLIENT_MULTI_STATEMENTS", INT2NUM(CLIENT_MULTI_STATEMENTS));
#endif
#ifdef CLIENT_MULTI_RESULTS
    rb_define_const(cMysql, "CLIENT_MULTI_RESULTS", INT2NUM(CLIENT_MULTI_RESULTS));
#endif
#if MYSQL_VERSION_ID >= 40101
    rb_define_const(cMysql, "OPTION_MULTI_STATEMENTS_ON", INT2NUM(MYSQL_OPTION_MULTI_STATEMENTS_ON));
    rb_define_const(cMysql, "OPTION_MULTI_STATEMENTS_OFF", INT2NUM(MYSQL_OPTION_MULTI_STATEMENTS_OFF));
#endif

    /* Mysql::Result object method */
    rb_define_method(cMysqlRes, "data_seek", data_seek, 1);
    rb_define_method(cMysqlRes, "fetch_field", fetch_field, 0);
    rb_define_method(cMysqlRes, "fetch_fields", fetch_fields, 0);
    rb_define_method(cMysqlRes, "fetch_field_direct", fetch_field_direct, 1);
    rb_define_method(cMysqlRes, "fetch_lengths", fetch_lengths, 0);
    rb_define_method(cMysqlRes, "fetch_row", fetch_row, 0);
    rb_define_method(cMysqlRes, "fetch_hash", fetch_hash, -1);
    rb_define_method(cMysqlRes, "field_seek", field_seek, 1);
    rb_define_method(cMysqlRes, "field_tell", field_tell, 0);
    rb_define_method(cMysqlRes, "free", res_free, 0);
    rb_define_method(cMysqlRes, "num_fields", num_fields, 0);
    rb_define_method(cMysqlRes, "num_rows", num_rows, 0);
    rb_define_method(cMysqlRes, "row_seek", row_seek, 1);
    rb_define_method(cMysqlRes, "row_tell", row_tell, 0);
    rb_define_method(cMysqlRes, "each", each, 0);
    rb_define_method(cMysqlRes, "each_hash", each_hash, -1);

    /* MysqlField object method */
    rb_define_method(cMysqlField, "name", field_name, 0);
    rb_define_method(cMysqlField, "table", field_table, 0);
    rb_define_method(cMysqlField, "def", field_def, 0);
    rb_define_method(cMysqlField, "type", field_type, 0);
    rb_define_method(cMysqlField, "length", field_length, 0);
    rb_define_method(cMysqlField, "max_length", field_max_length, 0);
    rb_define_method(cMysqlField, "flags", field_flags, 0);
    rb_define_method(cMysqlField, "decimals", field_decimals, 0);
    rb_define_method(cMysqlField, "hash", field_hash, 0);
    rb_define_method(cMysqlField, "inspect", field_inspect, 0);
#ifdef IS_NUM
    rb_define_method(cMysqlField, "is_num?", field_is_num, 0);
#endif
#ifdef IS_NOT_NULL
    rb_define_method(cMysqlField, "is_not_null?", field_is_not_null, 0);
#endif
#ifdef IS_PRI_KEY
    rb_define_method(cMysqlField, "is_pri_key?", field_is_pri_key, 0);
#endif

    /* Mysql::Field constant: TYPE */
    rb_define_const(cMysqlField, "TYPE_TINY", INT2NUM(FIELD_TYPE_TINY));
#if MYSQL_VERSION_ID >= 32115
    rb_define_const(cMysqlField, "TYPE_ENUM", INT2NUM(FIELD_TYPE_ENUM));
#endif
    rb_define_const(cMysqlField, "TYPE_DECIMAL", INT2NUM(FIELD_TYPE_DECIMAL));
    rb_define_const(cMysqlField, "TYPE_SHORT", INT2NUM(FIELD_TYPE_SHORT));
    rb_define_const(cMysqlField, "TYPE_LONG", INT2NUM(FIELD_TYPE_LONG));
    rb_define_const(cMysqlField, "TYPE_FLOAT", INT2NUM(FIELD_TYPE_FLOAT));
    rb_define_const(cMysqlField, "TYPE_DOUBLE", INT2NUM(FIELD_TYPE_DOUBLE));
    rb_define_const(cMysqlField, "TYPE_NULL", INT2NUM(FIELD_TYPE_NULL));
    rb_define_const(cMysqlField, "TYPE_TIMESTAMP", INT2NUM(FIELD_TYPE_TIMESTAMP));
    rb_define_const(cMysqlField, "TYPE_LONGLONG", INT2NUM(FIELD_TYPE_LONGLONG));
    rb_define_const(cMysqlField, "TYPE_INT24", INT2NUM(FIELD_TYPE_INT24));
    rb_define_const(cMysqlField, "TYPE_DATE", INT2NUM(FIELD_TYPE_DATE));
    rb_define_const(cMysqlField, "TYPE_TIME", INT2NUM(FIELD_TYPE_TIME));
    rb_define_const(cMysqlField, "TYPE_DATETIME", INT2NUM(FIELD_TYPE_DATETIME));
#if MYSQL_VERSION_ID >= 32130
    rb_define_const(cMysqlField, "TYPE_YEAR", INT2NUM(FIELD_TYPE_YEAR));
#endif
    rb_define_const(cMysqlField, "TYPE_SET", INT2NUM(FIELD_TYPE_SET));
    rb_define_const(cMysqlField, "TYPE_BLOB", INT2NUM(FIELD_TYPE_BLOB));
    rb_define_const(cMysqlField, "TYPE_STRING", INT2NUM(FIELD_TYPE_STRING));
#if MYSQL_VERSION_ID >= 40000
    rb_define_const(cMysqlField, "TYPE_VAR_STRING", INT2NUM(FIELD_TYPE_VAR_STRING));
#endif
    rb_define_const(cMysqlField, "TYPE_CHAR", INT2NUM(FIELD_TYPE_CHAR));

    /* Mysql::Field constant: FLAG */
    rb_define_const(cMysqlField, "NOT_NULL_FLAG", INT2NUM(NOT_NULL_FLAG));
    rb_define_const(cMysqlField, "PRI_KEY_FLAG", INT2NUM(PRI_KEY_FLAG));
    rb_define_const(cMysqlField, "UNIQUE_KEY_FLAG", INT2NUM(UNIQUE_KEY_FLAG));
    rb_define_const(cMysqlField, "MULTIPLE_KEY_FLAG", INT2NUM(MULTIPLE_KEY_FLAG));
    rb_define_const(cMysqlField, "BLOB_FLAG", INT2NUM(BLOB_FLAG));
    rb_define_const(cMysqlField, "UNSIGNED_FLAG", INT2NUM(UNSIGNED_FLAG));
    rb_define_const(cMysqlField, "ZEROFILL_FLAG", INT2NUM(ZEROFILL_FLAG));
    rb_define_const(cMysqlField, "BINARY_FLAG", INT2NUM(BINARY_FLAG));
#ifdef ENUM_FLAG
    rb_define_const(cMysqlField, "ENUM_FLAG", INT2NUM(ENUM_FLAG));
#endif
#ifdef AUTO_INCREMENT_FLAG
    rb_define_const(cMysqlField, "AUTO_INCREMENT_FLAG", INT2NUM(AUTO_INCREMENT_FLAG));
#endif
#ifdef TIMESTAMP_FLAG
    rb_define_const(cMysqlField, "TIMESTAMP_FLAG", INT2NUM(TIMESTAMP_FLAG));
#endif
#ifdef SET_FLAG
    rb_define_const(cMysqlField, "SET_FLAG", INT2NUM(SET_FLAG));
#endif
#ifdef NUM_FLAG
    rb_define_const(cMysqlField, "NUM_FLAG", INT2NUM(NUM_FLAG));
#endif
#ifdef PART_KEY_FLAG
    rb_define_const(cMysqlField, "PART_KEY_FLAG", INT2NUM(PART_KEY_FLAG));
#endif

#if MYSQL_VERSION_ID >= 40102
    /* Mysql::Stmt object method */
    rb_define_method(cMysqlStmt, "affected_rows", stmt_affected_rows, 0);
#if 0
    rb_define_method(cMysqlStmt, "attr_get", stmt_attr_get, 1);
    rb_define_method(cMysqlStmt, "attr_set", stmt_attr_set, 2);
#endif
    rb_define_method(cMysqlStmt, "bind_result", stmt_bind_result, -1);
    rb_define_method(cMysqlStmt, "close", stmt_close, 0);
    rb_define_method(cMysqlStmt, "data_seek", stmt_data_seek, 1);
    rb_define_method(cMysqlStmt, "each", stmt_each, 0);
    rb_define_method(cMysqlStmt, "execute", stmt_execute, -1);
    rb_define_method(cMysqlStmt, "fetch", stmt_fetch, 0);
    rb_define_method(cMysqlStmt, "field_count", stmt_field_count, 0);
    rb_define_method(cMysqlStmt, "free_result", stmt_free_result, 0);
    rb_define_method(cMysqlStmt, "insert_id", stmt_insert_id, 0);
    rb_define_method(cMysqlStmt, "num_rows", stmt_num_rows, 0);
    rb_define_method(cMysqlStmt, "param_count", stmt_param_count, 0);
    rb_define_method(cMysqlStmt, "prepare", stmt_prepare, 1);
#if 0
    rb_define_method(cMysqlStmt, "reset", stmt_reset, 0);
#endif
    rb_define_method(cMysqlStmt, "result_metadata", stmt_result_metadata, 0);
    rb_define_method(cMysqlStmt, "row_seek", stmt_row_seek, 1);
    rb_define_method(cMysqlStmt, "row_tell", stmt_row_tell, 0);
#if 0
    rb_define_method(cMysqlStmt, "send_long_data", stmt_send_long_data, 2);
#endif
    rb_define_method(cMysqlStmt, "sqlstate", stmt_sqlstate, 0);

#if 0
    rb_define_const(cMysqlStmt, "ATTR_UPDATE_MAX_LENGTH", INT2NUM(STMT_ATTR_UPDATE_MAX_LENGTH));
#endif

    /* Mysql::Time object method */
    rb_define_method(cMysqlTime, "initialize", time_initialize, -1);
    rb_define_method(cMysqlTime, "inspect", time_inspect, 0);
    rb_define_method(cMysqlTime, "to_s", time_to_s, 0);
    rb_define_method(cMysqlTime, "year", time_get_year, 0);
    rb_define_method(cMysqlTime, "month", time_get_month, 0);
    rb_define_method(cMysqlTime, "day", time_get_day, 0);
    rb_define_method(cMysqlTime, "hour", time_get_hour, 0);
    rb_define_method(cMysqlTime, "minute", time_get_minute, 0);
    rb_define_method(cMysqlTime, "second", time_get_second, 0);
    rb_define_method(cMysqlTime, "neg", time_get_neg, 0);
    rb_define_method(cMysqlTime, "second_part", time_get_second_part, 0);
    rb_define_method(cMysqlTime, "year=", time_set_year, 1);
    rb_define_method(cMysqlTime, "month=", time_set_month, 1);
    rb_define_method(cMysqlTime, "day=", time_set_day, 1);
    rb_define_method(cMysqlTime, "hour=", time_set_hour, 1);
    rb_define_method(cMysqlTime, "minute=", time_set_minute, 1);
    rb_define_method(cMysqlTime, "second=", time_set_second, 1);
    rb_define_method(cMysqlTime, "neg=", time_set_neg, 1);
    rb_define_method(cMysqlTime, "second_part=", time_set_second_part, 1);
    rb_define_method(cMysqlTime, "==", time_equal, 1);

#endif

    /* Mysql::Error object method */
    rb_define_method(eMysql, "error", error_error, 0);
    rb_define_method(eMysql, "errno", error_errno, 0);
    rb_define_method(eMysql, "sqlstate", error_sqlstate, 0);

    /* Mysql::Error constant */
    rb_define_const(eMysql, "CR_MIN_ERROR", INT2NUM(CR_MIN_ERROR));
    rb_define_const(eMysql, "CR_MAX_ERROR", INT2NUM(CR_MAX_ERROR));
    rb_define_const(eMysql, "CR_ERROR_FIRST", INT2NUM(CR_ERROR_FIRST));
    rb_define_const(eMysql, "CR_UNKNOWN_ERROR", INT2NUM(CR_UNKNOWN_ERROR));
    rb_define_const(eMysql, "CR_SOCKET_CREATE_ERROR", INT2NUM(CR_SOCKET_CREATE_ERROR));
    rb_define_const(eMysql, "CR_CONNECTION_ERROR", INT2NUM(CR_CONNECTION_ERROR));
    rb_define_const(eMysql, "CR_CONN_HOST_ERROR", INT2NUM(CR_CONN_HOST_ERROR));
    rb_define_const(eMysql, "CR_IPSOCK_ERROR", INT2NUM(CR_IPSOCK_ERROR));
    rb_define_const(eMysql, "CR_UNKNOWN_HOST", INT2NUM(CR_UNKNOWN_HOST));
    rb_define_const(eMysql, "CR_SERVER_GONE_ERROR", INT2NUM(CR_SERVER_GONE_ERROR));
    rb_define_const(eMysql, "CR_VERSION_ERROR", INT2NUM(CR_VERSION_ERROR));
    rb_define_const(eMysql, "CR_OUT_OF_MEMORY", INT2NUM(CR_OUT_OF_MEMORY));
    rb_define_const(eMysql, "CR_WRONG_HOST_INFO", INT2NUM(CR_WRONG_HOST_INFO));
    rb_define_const(eMysql, "CR_LOCALHOST_CONNECTION", INT2NUM(CR_LOCALHOST_CONNECTION));
    rb_define_const(eMysql, "CR_TCP_CONNECTION", INT2NUM(CR_TCP_CONNECTION));
    rb_define_const(eMysql, "CR_SERVER_HANDSHAKE_ERR", INT2NUM(CR_SERVER_HANDSHAKE_ERR));
    rb_define_const(eMysql, "CR_SERVER_LOST", INT2NUM(CR_SERVER_LOST));
    rb_define_const(eMysql, "CR_COMMANDS_OUT_OF_SYNC", INT2NUM(CR_COMMANDS_OUT_OF_SYNC));
    rb_define_const(eMysql, "CR_NAMEDPIPE_CONNECTION", INT2NUM(CR_NAMEDPIPE_CONNECTION));
    rb_define_const(eMysql, "CR_NAMEDPIPEWAIT_ERROR", INT2NUM(CR_NAMEDPIPEWAIT_ERROR));
    rb_define_const(eMysql, "CR_NAMEDPIPEOPEN_ERROR", INT2NUM(CR_NAMEDPIPEOPEN_ERROR));
    rb_define_const(eMysql, "CR_NAMEDPIPESETSTATE_ERROR", INT2NUM(CR_NAMEDPIPESETSTATE_ERROR));
    rb_define_const(eMysql, "CR_CANT_READ_CHARSET", INT2NUM(CR_CANT_READ_CHARSET));
    rb_define_const(eMysql, "CR_NET_PACKET_TOO_LARGE", INT2NUM(CR_NET_PACKET_TOO_LARGE));
    rb_define_const(eMysql, "CR_EMBEDDED_CONNECTION", INT2NUM(CR_EMBEDDED_CONNECTION));
    rb_define_const(eMysql, "CR_PROBE_SLAVE_STATUS", INT2NUM(CR_PROBE_SLAVE_STATUS));
    rb_define_const(eMysql, "CR_PROBE_SLAVE_HOSTS", INT2NUM(CR_PROBE_SLAVE_HOSTS));
    rb_define_const(eMysql, "CR_PROBE_SLAVE_CONNECT", INT2NUM(CR_PROBE_SLAVE_CONNECT));
    rb_define_const(eMysql, "CR_PROBE_MASTER_CONNECT", INT2NUM(CR_PROBE_MASTER_CONNECT));
    rb_define_const(eMysql, "CR_SSL_CONNECTION_ERROR", INT2NUM(CR_SSL_CONNECTION_ERROR));
    rb_define_const(eMysql, "CR_MALFORMED_PACKET", INT2NUM(CR_MALFORMED_PACKET));
    rb_define_const(eMysql, "CR_WRONG_LICENSE", INT2NUM(CR_WRONG_LICENSE));
    rb_define_const(eMysql, "CR_NULL_POINTER", INT2NUM(CR_NULL_POINTER));
    rb_define_const(eMysql, "CR_NO_PREPARE_STMT", INT2NUM(CR_NO_PREPARE_STMT));
    rb_define_const(eMysql, "CR_PARAMS_NOT_BOUND", INT2NUM(CR_PARAMS_NOT_BOUND));
    rb_define_const(eMysql, "CR_DATA_TRUNCATED", INT2NUM(CR_DATA_TRUNCATED));
    rb_define_const(eMysql, "CR_NO_PARAMETERS_EXISTS", INT2NUM(CR_NO_PARAMETERS_EXISTS));
    rb_define_const(eMysql, "CR_INVALID_PARAMETER_NO", INT2NUM(CR_INVALID_PARAMETER_NO));
    rb_define_const(eMysql, "CR_INVALID_BUFFER_USE", INT2NUM(CR_INVALID_BUFFER_USE));
    rb_define_const(eMysql, "CR_UNSUPPORTED_PARAM_TYPE", INT2NUM(CR_UNSUPPORTED_PARAM_TYPE));
    rb_define_const(eMysql, "CR_SHARED_MEMORY_CONNECTION", INT2NUM(CR_SHARED_MEMORY_CONNECTION));
    rb_define_const(eMysql, "CR_SHARED_MEMORY_CONNECT_REQUEST_ERROR", INT2NUM(CR_SHARED_MEMORY_CONNECT_REQUEST_ERROR));
    rb_define_const(eMysql, "CR_SHARED_MEMORY_CONNECT_ANSWER_ERROR", INT2NUM(CR_SHARED_MEMORY_CONNECT_ANSWER_ERROR));
    rb_define_const(eMysql, "CR_SHARED_MEMORY_CONNECT_FILE_MAP_ERROR", INT2NUM(CR_SHARED_MEMORY_CONNECT_FILE_MAP_ERROR));
    rb_define_const(eMysql, "CR_SHARED_MEMORY_CONNECT_MAP_ERROR", INT2NUM(CR_SHARED_MEMORY_CONNECT_MAP_ERROR));
    rb_define_const(eMysql, "CR_SHARED_MEMORY_FILE_MAP_ERROR", INT2NUM(CR_SHARED_MEMORY_FILE_MAP_ERROR));
    rb_define_const(eMysql, "CR_SHARED_MEMORY_MAP_ERROR", INT2NUM(CR_SHARED_MEMORY_MAP_ERROR));
    rb_define_const(eMysql, "CR_SHARED_MEMORY_EVENT_ERROR", INT2NUM(CR_SHARED_MEMORY_EVENT_ERROR));
    rb_define_const(eMysql, "CR_SHARED_MEMORY_CONNECT_ABANDONED_ERROR", INT2NUM(CR_SHARED_MEMORY_CONNECT_ABANDONED_ERROR));
    rb_define_const(eMysql, "CR_SHARED_MEMORY_CONNECT_SET_ERROR", INT2NUM(CR_SHARED_MEMORY_CONNECT_SET_ERROR));
    rb_define_const(eMysql, "CR_CONN_UNKNOW_PROTOCOL", INT2NUM(CR_CONN_UNKNOW_PROTOCOL));
    rb_define_const(eMysql, "CR_INVALID_CONN_HANDLE", INT2NUM(CR_INVALID_CONN_HANDLE));
    rb_define_const(eMysql, "CR_SECURE_AUTH", INT2NUM(CR_SECURE_AUTH));
    rb_define_const(eMysql, "CR_FETCH_CANCELED", INT2NUM(CR_FETCH_CANCELED));
    rb_define_const(eMysql, "CR_NO_DATA", INT2NUM(CR_NO_DATA));
    rb_define_const(eMysql, "CR_NO_STMT_METADATA", INT2NUM(CR_NO_STMT_METADATA));
    rb_define_const(eMysql, "CR_NO_RESULT_SET", INT2NUM(CR_NO_RESULT_SET));
    rb_define_const(eMysql, "CR_NOT_IMPLEMENTED", INT2NUM(CR_NOT_IMPLEMENTED));
    rb_define_const(eMysql, "CR_SERVER_LOST_EXTENDED", INT2NUM(CR_SERVER_LOST_EXTENDED));
    rb_define_const(eMysql, "CR_STMT_CLOSED", INT2NUM(CR_STMT_CLOSED));
    rb_define_const(eMysql, "CR_NEW_STMT_METADATA", INT2NUM(CR_NEW_STMT_METADATA));
    rb_define_const(eMysql, "CR_ALREADY_CONNECTED", INT2NUM(CR_ALREADY_CONNECTED));
    rb_define_const(eMysql, "CR_AUTH_PLUGIN_CANNOT_LOAD", INT2NUM(CR_AUTH_PLUGIN_CANNOT_LOAD));
    rb_define_const(eMysql, "CR_ERROR_LAST", INT2NUM(CR_ERROR_LAST));
    rb_define_const(eMysql, "ER_ERROR_FIRST", INT2NUM(ER_ERROR_FIRST));
    rb_define_const(eMysql, "ER_HASHCHK", INT2NUM(ER_HASHCHK));
    rb_define_const(eMysql, "ER_NISAMCHK", INT2NUM(ER_NISAMCHK));
    rb_define_const(eMysql, "ER_NO", INT2NUM(ER_NO));
    rb_define_const(eMysql, "ER_YES", INT2NUM(ER_YES));
    rb_define_const(eMysql, "ER_CANT_CREATE_FILE", INT2NUM(ER_CANT_CREATE_FILE));
    rb_define_const(eMysql, "ER_CANT_CREATE_TABLE", INT2NUM(ER_CANT_CREATE_TABLE));
    rb_define_const(eMysql, "ER_CANT_CREATE_DB", INT2NUM(ER_CANT_CREATE_DB));
    rb_define_const(eMysql, "ER_DB_CREATE_EXISTS", INT2NUM(ER_DB_CREATE_EXISTS));
    rb_define_const(eMysql, "ER_DB_DROP_EXISTS", INT2NUM(ER_DB_DROP_EXISTS));
    rb_define_const(eMysql, "ER_DB_DROP_DELETE", INT2NUM(ER_DB_DROP_DELETE));
    rb_define_const(eMysql, "ER_DB_DROP_RMDIR", INT2NUM(ER_DB_DROP_RMDIR));
    rb_define_const(eMysql, "ER_CANT_DELETE_FILE", INT2NUM(ER_CANT_DELETE_FILE));
    rb_define_const(eMysql, "ER_CANT_FIND_SYSTEM_REC", INT2NUM(ER_CANT_FIND_SYSTEM_REC));
    rb_define_const(eMysql, "ER_CANT_GET_STAT", INT2NUM(ER_CANT_GET_STAT));
    rb_define_const(eMysql, "ER_CANT_GET_WD", INT2NUM(ER_CANT_GET_WD));
    rb_define_const(eMysql, "ER_CANT_LOCK", INT2NUM(ER_CANT_LOCK));
    rb_define_const(eMysql, "ER_CANT_OPEN_FILE", INT2NUM(ER_CANT_OPEN_FILE));
    rb_define_const(eMysql, "ER_FILE_NOT_FOUND", INT2NUM(ER_FILE_NOT_FOUND));
    rb_define_const(eMysql, "ER_CANT_READ_DIR", INT2NUM(ER_CANT_READ_DIR));
    rb_define_const(eMysql, "ER_CANT_SET_WD", INT2NUM(ER_CANT_SET_WD));
    rb_define_const(eMysql, "ER_CHECKREAD", INT2NUM(ER_CHECKREAD));
    rb_define_const(eMysql, "ER_DISK_FULL", INT2NUM(ER_DISK_FULL));
    rb_define_const(eMysql, "ER_DUP_KEY", INT2NUM(ER_DUP_KEY));
    rb_define_const(eMysql, "ER_ERROR_ON_CLOSE", INT2NUM(ER_ERROR_ON_CLOSE));
    rb_define_const(eMysql, "ER_ERROR_ON_READ", INT2NUM(ER_ERROR_ON_READ));
    rb_define_const(eMysql, "ER_ERROR_ON_RENAME", INT2NUM(ER_ERROR_ON_RENAME));
    rb_define_const(eMysql, "ER_ERROR_ON_WRITE", INT2NUM(ER_ERROR_ON_WRITE));
    rb_define_const(eMysql, "ER_FILE_USED", INT2NUM(ER_FILE_USED));
    rb_define_const(eMysql, "ER_FILSORT_ABORT", INT2NUM(ER_FILSORT_ABORT));
    rb_define_const(eMysql, "ER_FORM_NOT_FOUND", INT2NUM(ER_FORM_NOT_FOUND));
    rb_define_const(eMysql, "ER_GET_ERRNO", INT2NUM(ER_GET_ERRNO));
    rb_define_const(eMysql, "ER_ILLEGAL_HA", INT2NUM(ER_ILLEGAL_HA));
    rb_define_const(eMysql, "ER_KEY_NOT_FOUND", INT2NUM(ER_KEY_NOT_FOUND));
    rb_define_const(eMysql, "ER_NOT_FORM_FILE", INT2NUM(ER_NOT_FORM_FILE));
    rb_define_const(eMysql, "ER_NOT_KEYFILE", INT2NUM(ER_NOT_KEYFILE));
    rb_define_const(eMysql, "ER_OLD_KEYFILE", INT2NUM(ER_OLD_KEYFILE));
    rb_define_const(eMysql, "ER_OPEN_AS_READONLY", INT2NUM(ER_OPEN_AS_READONLY));
    rb_define_const(eMysql, "ER_OUTOFMEMORY", INT2NUM(ER_OUTOFMEMORY));
    rb_define_const(eMysql, "ER_OUT_OF_SORTMEMORY", INT2NUM(ER_OUT_OF_SORTMEMORY));
    rb_define_const(eMysql, "ER_UNEXPECTED_EOF", INT2NUM(ER_UNEXPECTED_EOF));
    rb_define_const(eMysql, "ER_CON_COUNT_ERROR", INT2NUM(ER_CON_COUNT_ERROR));
    rb_define_const(eMysql, "ER_OUT_OF_RESOURCES", INT2NUM(ER_OUT_OF_RESOURCES));
    rb_define_const(eMysql, "ER_BAD_HOST_ERROR", INT2NUM(ER_BAD_HOST_ERROR));
    rb_define_const(eMysql, "ER_HANDSHAKE_ERROR", INT2NUM(ER_HANDSHAKE_ERROR));
    rb_define_const(eMysql, "ER_DBACCESS_DENIED_ERROR", INT2NUM(ER_DBACCESS_DENIED_ERROR));
    rb_define_const(eMysql, "ER_ACCESS_DENIED_ERROR", INT2NUM(ER_ACCESS_DENIED_ERROR));
    rb_define_const(eMysql, "ER_NO_DB_ERROR", INT2NUM(ER_NO_DB_ERROR));
    rb_define_const(eMysql, "ER_UNKNOWN_COM_ERROR", INT2NUM(ER_UNKNOWN_COM_ERROR));
    rb_define_const(eMysql, "ER_BAD_NULL_ERROR", INT2NUM(ER_BAD_NULL_ERROR));
    rb_define_const(eMysql, "ER_BAD_DB_ERROR", INT2NUM(ER_BAD_DB_ERROR));
    rb_define_const(eMysql, "ER_TABLE_EXISTS_ERROR", INT2NUM(ER_TABLE_EXISTS_ERROR));
    rb_define_const(eMysql, "ER_BAD_TABLE_ERROR", INT2NUM(ER_BAD_TABLE_ERROR));
    rb_define_const(eMysql, "ER_NON_UNIQ_ERROR", INT2NUM(ER_NON_UNIQ_ERROR));
    rb_define_const(eMysql, "ER_SERVER_SHUTDOWN", INT2NUM(ER_SERVER_SHUTDOWN));
    rb_define_const(eMysql, "ER_BAD_FIELD_ERROR", INT2NUM(ER_BAD_FIELD_ERROR));
    rb_define_const(eMysql, "ER_WRONG_FIELD_WITH_GROUP", INT2NUM(ER_WRONG_FIELD_WITH_GROUP));
    rb_define_const(eMysql, "ER_WRONG_GROUP_FIELD", INT2NUM(ER_WRONG_GROUP_FIELD));
    rb_define_const(eMysql, "ER_WRONG_SUM_SELECT", INT2NUM(ER_WRONG_SUM_SELECT));
    rb_define_const(eMysql, "ER_WRONG_VALUE_COUNT", INT2NUM(ER_WRONG_VALUE_COUNT));
    rb_define_const(eMysql, "ER_TOO_LONG_IDENT", INT2NUM(ER_TOO_LONG_IDENT));
    rb_define_const(eMysql, "ER_DUP_FIELDNAME", INT2NUM(ER_DUP_FIELDNAME));
    rb_define_const(eMysql, "ER_DUP_KEYNAME", INT2NUM(ER_DUP_KEYNAME));
    rb_define_const(eMysql, "ER_DUP_ENTRY", INT2NUM(ER_DUP_ENTRY));
    rb_define_const(eMysql, "ER_WRONG_FIELD_SPEC", INT2NUM(ER_WRONG_FIELD_SPEC));
    rb_define_const(eMysql, "ER_PARSE_ERROR", INT2NUM(ER_PARSE_ERROR));
    rb_define_const(eMysql, "ER_EMPTY_QUERY", INT2NUM(ER_EMPTY_QUERY));
    rb_define_const(eMysql, "ER_NONUNIQ_TABLE", INT2NUM(ER_NONUNIQ_TABLE));
    rb_define_const(eMysql, "ER_INVALID_DEFAULT", INT2NUM(ER_INVALID_DEFAULT));
    rb_define_const(eMysql, "ER_MULTIPLE_PRI_KEY", INT2NUM(ER_MULTIPLE_PRI_KEY));
    rb_define_const(eMysql, "ER_TOO_MANY_KEYS", INT2NUM(ER_TOO_MANY_KEYS));
    rb_define_const(eMysql, "ER_TOO_MANY_KEY_PARTS", INT2NUM(ER_TOO_MANY_KEY_PARTS));
    rb_define_const(eMysql, "ER_TOO_LONG_KEY", INT2NUM(ER_TOO_LONG_KEY));
    rb_define_const(eMysql, "ER_KEY_COLUMN_DOES_NOT_EXITS", INT2NUM(ER_KEY_COLUMN_DOES_NOT_EXITS));
    rb_define_const(eMysql, "ER_BLOB_USED_AS_KEY", INT2NUM(ER_BLOB_USED_AS_KEY));
    rb_define_const(eMysql, "ER_TOO_BIG_FIELDLENGTH", INT2NUM(ER_TOO_BIG_FIELDLENGTH));
    rb_define_const(eMysql, "ER_WRONG_AUTO_KEY", INT2NUM(ER_WRONG_AUTO_KEY));
    rb_define_const(eMysql, "ER_READY", INT2NUM(ER_READY));
    rb_define_const(eMysql, "ER_NORMAL_SHUTDOWN", INT2NUM(ER_NORMAL_SHUTDOWN));
    rb_define_const(eMysql, "ER_GOT_SIGNAL", INT2NUM(ER_GOT_SIGNAL));
    rb_define_const(eMysql, "ER_SHUTDOWN_COMPLETE", INT2NUM(ER_SHUTDOWN_COMPLETE));
    rb_define_const(eMysql, "ER_FORCING_CLOSE", INT2NUM(ER_FORCING_CLOSE));
    rb_define_const(eMysql, "ER_IPSOCK_ERROR", INT2NUM(ER_IPSOCK_ERROR));
    rb_define_const(eMysql, "ER_NO_SUCH_INDEX", INT2NUM(ER_NO_SUCH_INDEX));
    rb_define_const(eMysql, "ER_WRONG_FIELD_TERMINATORS", INT2NUM(ER_WRONG_FIELD_TERMINATORS));
    rb_define_const(eMysql, "ER_BLOBS_AND_NO_TERMINATED", INT2NUM(ER_BLOBS_AND_NO_TERMINATED));
    rb_define_const(eMysql, "ER_TEXTFILE_NOT_READABLE", INT2NUM(ER_TEXTFILE_NOT_READABLE));
    rb_define_const(eMysql, "ER_FILE_EXISTS_ERROR", INT2NUM(ER_FILE_EXISTS_ERROR));
    rb_define_const(eMysql, "ER_LOAD_INFO", INT2NUM(ER_LOAD_INFO));
    rb_define_const(eMysql, "ER_ALTER_INFO", INT2NUM(ER_ALTER_INFO));
    rb_define_const(eMysql, "ER_WRONG_SUB_KEY", INT2NUM(ER_WRONG_SUB_KEY));
    rb_define_const(eMysql, "ER_CANT_REMOVE_ALL_FIELDS", INT2NUM(ER_CANT_REMOVE_ALL_FIELDS));
    rb_define_const(eMysql, "ER_CANT_DROP_FIELD_OR_KEY", INT2NUM(ER_CANT_DROP_FIELD_OR_KEY));
    rb_define_const(eMysql, "ER_INSERT_INFO", INT2NUM(ER_INSERT_INFO));
    rb_define_const(eMysql, "ER_UPDATE_TABLE_USED", INT2NUM(ER_UPDATE_TABLE_USED));
    rb_define_const(eMysql, "ER_NO_SUCH_THREAD", INT2NUM(ER_NO_SUCH_THREAD));
    rb_define_const(eMysql, "ER_KILL_DENIED_ERROR", INT2NUM(ER_KILL_DENIED_ERROR));
    rb_define_const(eMysql, "ER_NO_TABLES_USED", INT2NUM(ER_NO_TABLES_USED));
    rb_define_const(eMysql, "ER_TOO_BIG_SET", INT2NUM(ER_TOO_BIG_SET));
    rb_define_const(eMysql, "ER_NO_UNIQUE_LOGFILE", INT2NUM(ER_NO_UNIQUE_LOGFILE));
    rb_define_const(eMysql, "ER_TABLE_NOT_LOCKED_FOR_WRITE", INT2NUM(ER_TABLE_NOT_LOCKED_FOR_WRITE));
    rb_define_const(eMysql, "ER_TABLE_NOT_LOCKED", INT2NUM(ER_TABLE_NOT_LOCKED));
    rb_define_const(eMysql, "ER_BLOB_CANT_HAVE_DEFAULT", INT2NUM(ER_BLOB_CANT_HAVE_DEFAULT));
    rb_define_const(eMysql, "ER_WRONG_DB_NAME", INT2NUM(ER_WRONG_DB_NAME));
    rb_define_const(eMysql, "ER_WRONG_TABLE_NAME", INT2NUM(ER_WRONG_TABLE_NAME));
    rb_define_const(eMysql, "ER_TOO_BIG_SELECT", INT2NUM(ER_TOO_BIG_SELECT));
    rb_define_const(eMysql, "ER_UNKNOWN_ERROR", INT2NUM(ER_UNKNOWN_ERROR));
    rb_define_const(eMysql, "ER_UNKNOWN_PROCEDURE", INT2NUM(ER_UNKNOWN_PROCEDURE));
    rb_define_const(eMysql, "ER_WRONG_PARAMCOUNT_TO_PROCEDURE", INT2NUM(ER_WRONG_PARAMCOUNT_TO_PROCEDURE));
    rb_define_const(eMysql, "ER_WRONG_PARAMETERS_TO_PROCEDURE", INT2NUM(ER_WRONG_PARAMETERS_TO_PROCEDURE));
    rb_define_const(eMysql, "ER_UNKNOWN_TABLE", INT2NUM(ER_UNKNOWN_TABLE));
    rb_define_const(eMysql, "ER_FIELD_SPECIFIED_TWICE", INT2NUM(ER_FIELD_SPECIFIED_TWICE));
    rb_define_const(eMysql, "ER_INVALID_GROUP_FUNC_USE", INT2NUM(ER_INVALID_GROUP_FUNC_USE));
    rb_define_const(eMysql, "ER_UNSUPPORTED_EXTENSION", INT2NUM(ER_UNSUPPORTED_EXTENSION));
    rb_define_const(eMysql, "ER_TABLE_MUST_HAVE_COLUMNS", INT2NUM(ER_TABLE_MUST_HAVE_COLUMNS));
    rb_define_const(eMysql, "ER_RECORD_FILE_FULL", INT2NUM(ER_RECORD_FILE_FULL));
    rb_define_const(eMysql, "ER_UNKNOWN_CHARACTER_SET", INT2NUM(ER_UNKNOWN_CHARACTER_SET));
    rb_define_const(eMysql, "ER_TOO_MANY_TABLES", INT2NUM(ER_TOO_MANY_TABLES));
    rb_define_const(eMysql, "ER_TOO_MANY_FIELDS", INT2NUM(ER_TOO_MANY_FIELDS));
    rb_define_const(eMysql, "ER_TOO_BIG_ROWSIZE", INT2NUM(ER_TOO_BIG_ROWSIZE));
    rb_define_const(eMysql, "ER_STACK_OVERRUN", INT2NUM(ER_STACK_OVERRUN));
    rb_define_const(eMysql, "ER_WRONG_OUTER_JOIN", INT2NUM(ER_WRONG_OUTER_JOIN));
    rb_define_const(eMysql, "ER_NULL_COLUMN_IN_INDEX", INT2NUM(ER_NULL_COLUMN_IN_INDEX));
    rb_define_const(eMysql, "ER_CANT_FIND_UDF", INT2NUM(ER_CANT_FIND_UDF));
    rb_define_const(eMysql, "ER_CANT_INITIALIZE_UDF", INT2NUM(ER_CANT_INITIALIZE_UDF));
    rb_define_const(eMysql, "ER_UDF_NO_PATHS", INT2NUM(ER_UDF_NO_PATHS));
    rb_define_const(eMysql, "ER_UDF_EXISTS", INT2NUM(ER_UDF_EXISTS));
    rb_define_const(eMysql, "ER_CANT_OPEN_LIBRARY", INT2NUM(ER_CANT_OPEN_LIBRARY));
    rb_define_const(eMysql, "ER_CANT_FIND_DL_ENTRY", INT2NUM(ER_CANT_FIND_DL_ENTRY));
    rb_define_const(eMysql, "ER_FUNCTION_NOT_DEFINED", INT2NUM(ER_FUNCTION_NOT_DEFINED));
    rb_define_const(eMysql, "ER_HOST_IS_BLOCKED", INT2NUM(ER_HOST_IS_BLOCKED));
    rb_define_const(eMysql, "ER_HOST_NOT_PRIVILEGED", INT2NUM(ER_HOST_NOT_PRIVILEGED));
    rb_define_const(eMysql, "ER_PASSWORD_ANONYMOUS_USER", INT2NUM(ER_PASSWORD_ANONYMOUS_USER));
    rb_define_const(eMysql, "ER_PASSWORD_NOT_ALLOWED", INT2NUM(ER_PASSWORD_NOT_ALLOWED));
    rb_define_const(eMysql, "ER_PASSWORD_NO_MATCH", INT2NUM(ER_PASSWORD_NO_MATCH));
    rb_define_const(eMysql, "ER_UPDATE_INFO", INT2NUM(ER_UPDATE_INFO));
    rb_define_const(eMysql, "ER_CANT_CREATE_THREAD", INT2NUM(ER_CANT_CREATE_THREAD));
    rb_define_const(eMysql, "ER_WRONG_VALUE_COUNT_ON_ROW", INT2NUM(ER_WRONG_VALUE_COUNT_ON_ROW));
    rb_define_const(eMysql, "ER_CANT_REOPEN_TABLE", INT2NUM(ER_CANT_REOPEN_TABLE));
    rb_define_const(eMysql, "ER_INVALID_USE_OF_NULL", INT2NUM(ER_INVALID_USE_OF_NULL));
    rb_define_const(eMysql, "ER_REGEXP_ERROR", INT2NUM(ER_REGEXP_ERROR));
    rb_define_const(eMysql, "ER_MIX_OF_GROUP_FUNC_AND_FIELDS", INT2NUM(ER_MIX_OF_GROUP_FUNC_AND_FIELDS));
    rb_define_const(eMysql, "ER_NONEXISTING_GRANT", INT2NUM(ER_NONEXISTING_GRANT));
    rb_define_const(eMysql, "ER_TABLEACCESS_DENIED_ERROR", INT2NUM(ER_TABLEACCESS_DENIED_ERROR));
    rb_define_const(eMysql, "ER_COLUMNACCESS_DENIED_ERROR", INT2NUM(ER_COLUMNACCESS_DENIED_ERROR));
    rb_define_const(eMysql, "ER_ILLEGAL_GRANT_FOR_TABLE", INT2NUM(ER_ILLEGAL_GRANT_FOR_TABLE));
    rb_define_const(eMysql, "ER_GRANT_WRONG_HOST_OR_USER", INT2NUM(ER_GRANT_WRONG_HOST_OR_USER));
    rb_define_const(eMysql, "ER_NO_SUCH_TABLE", INT2NUM(ER_NO_SUCH_TABLE));
    rb_define_const(eMysql, "ER_NONEXISTING_TABLE_GRANT", INT2NUM(ER_NONEXISTING_TABLE_GRANT));
    rb_define_const(eMysql, "ER_NOT_ALLOWED_COMMAND", INT2NUM(ER_NOT_ALLOWED_COMMAND));
    rb_define_const(eMysql, "ER_SYNTAX_ERROR", INT2NUM(ER_SYNTAX_ERROR));
    rb_define_const(eMysql, "ER_DELAYED_CANT_CHANGE_LOCK", INT2NUM(ER_DELAYED_CANT_CHANGE_LOCK));
    rb_define_const(eMysql, "ER_TOO_MANY_DELAYED_THREADS", INT2NUM(ER_TOO_MANY_DELAYED_THREADS));
    rb_define_const(eMysql, "ER_ABORTING_CONNECTION", INT2NUM(ER_ABORTING_CONNECTION));
    rb_define_const(eMysql, "ER_NET_PACKET_TOO_LARGE", INT2NUM(ER_NET_PACKET_TOO_LARGE));
    rb_define_const(eMysql, "ER_NET_READ_ERROR_FROM_PIPE", INT2NUM(ER_NET_READ_ERROR_FROM_PIPE));
    rb_define_const(eMysql, "ER_NET_FCNTL_ERROR", INT2NUM(ER_NET_FCNTL_ERROR));
    rb_define_const(eMysql, "ER_NET_PACKETS_OUT_OF_ORDER", INT2NUM(ER_NET_PACKETS_OUT_OF_ORDER));
    rb_define_const(eMysql, "ER_NET_UNCOMPRESS_ERROR", INT2NUM(ER_NET_UNCOMPRESS_ERROR));
    rb_define_const(eMysql, "ER_NET_READ_ERROR", INT2NUM(ER_NET_READ_ERROR));
    rb_define_const(eMysql, "ER_NET_READ_INTERRUPTED", INT2NUM(ER_NET_READ_INTERRUPTED));
    rb_define_const(eMysql, "ER_NET_ERROR_ON_WRITE", INT2NUM(ER_NET_ERROR_ON_WRITE));
    rb_define_const(eMysql, "ER_NET_WRITE_INTERRUPTED", INT2NUM(ER_NET_WRITE_INTERRUPTED));
    rb_define_const(eMysql, "ER_TOO_LONG_STRING", INT2NUM(ER_TOO_LONG_STRING));
    rb_define_const(eMysql, "ER_TABLE_CANT_HANDLE_BLOB", INT2NUM(ER_TABLE_CANT_HANDLE_BLOB));
    rb_define_const(eMysql, "ER_TABLE_CANT_HANDLE_AUTO_INCREMENT", INT2NUM(ER_TABLE_CANT_HANDLE_AUTO_INCREMENT));
    rb_define_const(eMysql, "ER_DELAYED_INSERT_TABLE_LOCKED", INT2NUM(ER_DELAYED_INSERT_TABLE_LOCKED));
    rb_define_const(eMysql, "ER_WRONG_COLUMN_NAME", INT2NUM(ER_WRONG_COLUMN_NAME));
    rb_define_const(eMysql, "ER_WRONG_KEY_COLUMN", INT2NUM(ER_WRONG_KEY_COLUMN));
    rb_define_const(eMysql, "ER_WRONG_MRG_TABLE", INT2NUM(ER_WRONG_MRG_TABLE));
    rb_define_const(eMysql, "ER_DUP_UNIQUE", INT2NUM(ER_DUP_UNIQUE));
    rb_define_const(eMysql, "ER_BLOB_KEY_WITHOUT_LENGTH", INT2NUM(ER_BLOB_KEY_WITHOUT_LENGTH));
    rb_define_const(eMysql, "ER_PRIMARY_CANT_HAVE_NULL", INT2NUM(ER_PRIMARY_CANT_HAVE_NULL));
    rb_define_const(eMysql, "ER_TOO_MANY_ROWS", INT2NUM(ER_TOO_MANY_ROWS));
    rb_define_const(eMysql, "ER_REQUIRES_PRIMARY_KEY", INT2NUM(ER_REQUIRES_PRIMARY_KEY));
    rb_define_const(eMysql, "ER_NO_RAID_COMPILED", INT2NUM(ER_NO_RAID_COMPILED));
    rb_define_const(eMysql, "ER_UPDATE_WITHOUT_KEY_IN_SAFE_MODE", INT2NUM(ER_UPDATE_WITHOUT_KEY_IN_SAFE_MODE));
    rb_define_const(eMysql, "ER_KEY_DOES_NOT_EXITS", INT2NUM(ER_KEY_DOES_NOT_EXITS));
    rb_define_const(eMysql, "ER_CHECK_NO_SUCH_TABLE", INT2NUM(ER_CHECK_NO_SUCH_TABLE));
    rb_define_const(eMysql, "ER_CHECK_NOT_IMPLEMENTED", INT2NUM(ER_CHECK_NOT_IMPLEMENTED));
    rb_define_const(eMysql, "ER_CANT_DO_THIS_DURING_AN_TRANSACTION", INT2NUM(ER_CANT_DO_THIS_DURING_AN_TRANSACTION));
    rb_define_const(eMysql, "ER_ERROR_DURING_COMMIT", INT2NUM(ER_ERROR_DURING_COMMIT));
    rb_define_const(eMysql, "ER_ERROR_DURING_ROLLBACK", INT2NUM(ER_ERROR_DURING_ROLLBACK));
    rb_define_const(eMysql, "ER_ERROR_DURING_FLUSH_LOGS", INT2NUM(ER_ERROR_DURING_FLUSH_LOGS));
    rb_define_const(eMysql, "ER_ERROR_DURING_CHECKPOINT", INT2NUM(ER_ERROR_DURING_CHECKPOINT));
    rb_define_const(eMysql, "ER_NEW_ABORTING_CONNECTION", INT2NUM(ER_NEW_ABORTING_CONNECTION));
    rb_define_const(eMysql, "ER_DUMP_NOT_IMPLEMENTED", INT2NUM(ER_DUMP_NOT_IMPLEMENTED));
    rb_define_const(eMysql, "ER_FLUSH_MASTER_BINLOG_CLOSED", INT2NUM(ER_FLUSH_MASTER_BINLOG_CLOSED));
    rb_define_const(eMysql, "ER_INDEX_REBUILD", INT2NUM(ER_INDEX_REBUILD));
    rb_define_const(eMysql, "ER_MASTER", INT2NUM(ER_MASTER));
    rb_define_const(eMysql, "ER_MASTER_NET_READ", INT2NUM(ER_MASTER_NET_READ));
    rb_define_const(eMysql, "ER_MASTER_NET_WRITE", INT2NUM(ER_MASTER_NET_WRITE));
    rb_define_const(eMysql, "ER_FT_MATCHING_KEY_NOT_FOUND", INT2NUM(ER_FT_MATCHING_KEY_NOT_FOUND));
    rb_define_const(eMysql, "ER_LOCK_OR_ACTIVE_TRANSACTION", INT2NUM(ER_LOCK_OR_ACTIVE_TRANSACTION));
    rb_define_const(eMysql, "ER_UNKNOWN_SYSTEM_VARIABLE", INT2NUM(ER_UNKNOWN_SYSTEM_VARIABLE));
    rb_define_const(eMysql, "ER_CRASHED_ON_USAGE", INT2NUM(ER_CRASHED_ON_USAGE));
    rb_define_const(eMysql, "ER_CRASHED_ON_REPAIR", INT2NUM(ER_CRASHED_ON_REPAIR));
    rb_define_const(eMysql, "ER_WARNING_NOT_COMPLETE_ROLLBACK", INT2NUM(ER_WARNING_NOT_COMPLETE_ROLLBACK));
    rb_define_const(eMysql, "ER_TRANS_CACHE_FULL", INT2NUM(ER_TRANS_CACHE_FULL));
    rb_define_const(eMysql, "ER_SLAVE_MUST_STOP", INT2NUM(ER_SLAVE_MUST_STOP));
    rb_define_const(eMysql, "ER_SLAVE_NOT_RUNNING", INT2NUM(ER_SLAVE_NOT_RUNNING));
    rb_define_const(eMysql, "ER_BAD_SLAVE", INT2NUM(ER_BAD_SLAVE));
    rb_define_const(eMysql, "ER_MASTER_INFO", INT2NUM(ER_MASTER_INFO));
    rb_define_const(eMysql, "ER_SLAVE_THREAD", INT2NUM(ER_SLAVE_THREAD));
    rb_define_const(eMysql, "ER_TOO_MANY_USER_CONNECTIONS", INT2NUM(ER_TOO_MANY_USER_CONNECTIONS));
    rb_define_const(eMysql, "ER_SET_CONSTANTS_ONLY", INT2NUM(ER_SET_CONSTANTS_ONLY));
    rb_define_const(eMysql, "ER_LOCK_WAIT_TIMEOUT", INT2NUM(ER_LOCK_WAIT_TIMEOUT));
    rb_define_const(eMysql, "ER_LOCK_TABLE_FULL", INT2NUM(ER_LOCK_TABLE_FULL));
    rb_define_const(eMysql, "ER_READ_ONLY_TRANSACTION", INT2NUM(ER_READ_ONLY_TRANSACTION));
    rb_define_const(eMysql, "ER_DROP_DB_WITH_READ_LOCK", INT2NUM(ER_DROP_DB_WITH_READ_LOCK));
    rb_define_const(eMysql, "ER_CREATE_DB_WITH_READ_LOCK", INT2NUM(ER_CREATE_DB_WITH_READ_LOCK));
    rb_define_const(eMysql, "ER_WRONG_ARGUMENTS", INT2NUM(ER_WRONG_ARGUMENTS));
    rb_define_const(eMysql, "ER_NO_PERMISSION_TO_CREATE_USER", INT2NUM(ER_NO_PERMISSION_TO_CREATE_USER));
    rb_define_const(eMysql, "ER_UNION_TABLES_IN_DIFFERENT_DIR", INT2NUM(ER_UNION_TABLES_IN_DIFFERENT_DIR));
    rb_define_const(eMysql, "ER_LOCK_DEADLOCK", INT2NUM(ER_LOCK_DEADLOCK));
    rb_define_const(eMysql, "ER_TABLE_CANT_HANDLE_FT", INT2NUM(ER_TABLE_CANT_HANDLE_FT));
    rb_define_const(eMysql, "ER_CANNOT_ADD_FOREIGN", INT2NUM(ER_CANNOT_ADD_FOREIGN));
    rb_define_const(eMysql, "ER_NO_REFERENCED_ROW", INT2NUM(ER_NO_REFERENCED_ROW));
    rb_define_const(eMysql, "ER_ROW_IS_REFERENCED", INT2NUM(ER_ROW_IS_REFERENCED));
    rb_define_const(eMysql, "ER_CONNECT_TO_MASTER", INT2NUM(ER_CONNECT_TO_MASTER));
    rb_define_const(eMysql, "ER_QUERY_ON_MASTER", INT2NUM(ER_QUERY_ON_MASTER));
    rb_define_const(eMysql, "ER_ERROR_WHEN_EXECUTING_COMMAND", INT2NUM(ER_ERROR_WHEN_EXECUTING_COMMAND));
    rb_define_const(eMysql, "ER_WRONG_USAGE", INT2NUM(ER_WRONG_USAGE));
    rb_define_const(eMysql, "ER_WRONG_NUMBER_OF_COLUMNS_IN_SELECT", INT2NUM(ER_WRONG_NUMBER_OF_COLUMNS_IN_SELECT));
    rb_define_const(eMysql, "ER_CANT_UPDATE_WITH_READLOCK", INT2NUM(ER_CANT_UPDATE_WITH_READLOCK));
    rb_define_const(eMysql, "ER_MIXING_NOT_ALLOWED", INT2NUM(ER_MIXING_NOT_ALLOWED));
    rb_define_const(eMysql, "ER_DUP_ARGUMENT", INT2NUM(ER_DUP_ARGUMENT));
    rb_define_const(eMysql, "ER_USER_LIMIT_REACHED", INT2NUM(ER_USER_LIMIT_REACHED));
    rb_define_const(eMysql, "ER_SPECIFIC_ACCESS_DENIED_ERROR", INT2NUM(ER_SPECIFIC_ACCESS_DENIED_ERROR));
    rb_define_const(eMysql, "ER_LOCAL_VARIABLE", INT2NUM(ER_LOCAL_VARIABLE));
    rb_define_const(eMysql, "ER_GLOBAL_VARIABLE", INT2NUM(ER_GLOBAL_VARIABLE));
    rb_define_const(eMysql, "ER_NO_DEFAULT", INT2NUM(ER_NO_DEFAULT));
    rb_define_const(eMysql, "ER_WRONG_VALUE_FOR_VAR", INT2NUM(ER_WRONG_VALUE_FOR_VAR));
    rb_define_const(eMysql, "ER_WRONG_TYPE_FOR_VAR", INT2NUM(ER_WRONG_TYPE_FOR_VAR));
    rb_define_const(eMysql, "ER_VAR_CANT_BE_READ", INT2NUM(ER_VAR_CANT_BE_READ));
    rb_define_const(eMysql, "ER_CANT_USE_OPTION_HERE", INT2NUM(ER_CANT_USE_OPTION_HERE));
    rb_define_const(eMysql, "ER_NOT_SUPPORTED_YET", INT2NUM(ER_NOT_SUPPORTED_YET));
    rb_define_const(eMysql, "ER_MASTER_FATAL_ERROR_READING_BINLOG", INT2NUM(ER_MASTER_FATAL_ERROR_READING_BINLOG));
    rb_define_const(eMysql, "ER_SLAVE_IGNORED_TABLE", INT2NUM(ER_SLAVE_IGNORED_TABLE));
    rb_define_const(eMysql, "ER_INCORRECT_GLOBAL_LOCAL_VAR", INT2NUM(ER_INCORRECT_GLOBAL_LOCAL_VAR));
    rb_define_const(eMysql, "ER_WRONG_FK_DEF", INT2NUM(ER_WRONG_FK_DEF));
    rb_define_const(eMysql, "ER_KEY_REF_DO_NOT_MATCH_TABLE_REF", INT2NUM(ER_KEY_REF_DO_NOT_MATCH_TABLE_REF));
    rb_define_const(eMysql, "ER_OPERAND_COLUMNS", INT2NUM(ER_OPERAND_COLUMNS));
    rb_define_const(eMysql, "ER_SUBQUERY_NO_1_ROW", INT2NUM(ER_SUBQUERY_NO_1_ROW));
    rb_define_const(eMysql, "ER_UNKNOWN_STMT_HANDLER", INT2NUM(ER_UNKNOWN_STMT_HANDLER));
    rb_define_const(eMysql, "ER_CORRUPT_HELP_DB", INT2NUM(ER_CORRUPT_HELP_DB));
    rb_define_const(eMysql, "ER_CYCLIC_REFERENCE", INT2NUM(ER_CYCLIC_REFERENCE));
    rb_define_const(eMysql, "ER_AUTO_CONVERT", INT2NUM(ER_AUTO_CONVERT));
    rb_define_const(eMysql, "ER_ILLEGAL_REFERENCE", INT2NUM(ER_ILLEGAL_REFERENCE));
    rb_define_const(eMysql, "ER_DERIVED_MUST_HAVE_ALIAS", INT2NUM(ER_DERIVED_MUST_HAVE_ALIAS));
    rb_define_const(eMysql, "ER_SELECT_REDUCED", INT2NUM(ER_SELECT_REDUCED));
    rb_define_const(eMysql, "ER_TABLENAME_NOT_ALLOWED_HERE", INT2NUM(ER_TABLENAME_NOT_ALLOWED_HERE));
    rb_define_const(eMysql, "ER_NOT_SUPPORTED_AUTH_MODE", INT2NUM(ER_NOT_SUPPORTED_AUTH_MODE));
    rb_define_const(eMysql, "ER_SPATIAL_CANT_HAVE_NULL", INT2NUM(ER_SPATIAL_CANT_HAVE_NULL));
    rb_define_const(eMysql, "ER_COLLATION_CHARSET_MISMATCH", INT2NUM(ER_COLLATION_CHARSET_MISMATCH));
    rb_define_const(eMysql, "ER_SLAVE_WAS_RUNNING", INT2NUM(ER_SLAVE_WAS_RUNNING));
    rb_define_const(eMysql, "ER_SLAVE_WAS_NOT_RUNNING", INT2NUM(ER_SLAVE_WAS_NOT_RUNNING));
    rb_define_const(eMysql, "ER_TOO_BIG_FOR_UNCOMPRESS", INT2NUM(ER_TOO_BIG_FOR_UNCOMPRESS));
    rb_define_const(eMysql, "ER_ZLIB_Z_MEM_ERROR", INT2NUM(ER_ZLIB_Z_MEM_ERROR));
    rb_define_const(eMysql, "ER_ZLIB_Z_BUF_ERROR", INT2NUM(ER_ZLIB_Z_BUF_ERROR));
    rb_define_const(eMysql, "ER_ZLIB_Z_DATA_ERROR", INT2NUM(ER_ZLIB_Z_DATA_ERROR));
    rb_define_const(eMysql, "ER_CUT_VALUE_GROUP_CONCAT", INT2NUM(ER_CUT_VALUE_GROUP_CONCAT));
    rb_define_const(eMysql, "ER_WARN_TOO_FEW_RECORDS", INT2NUM(ER_WARN_TOO_FEW_RECORDS));
    rb_define_const(eMysql, "ER_WARN_TOO_MANY_RECORDS", INT2NUM(ER_WARN_TOO_MANY_RECORDS));
    rb_define_const(eMysql, "ER_WARN_NULL_TO_NOTNULL", INT2NUM(ER_WARN_NULL_TO_NOTNULL));
    rb_define_const(eMysql, "ER_WARN_DATA_OUT_OF_RANGE", INT2NUM(ER_WARN_DATA_OUT_OF_RANGE));
    rb_define_const(eMysql, "ER_WARN_USING_OTHER_HANDLER", INT2NUM(ER_WARN_USING_OTHER_HANDLER));
    rb_define_const(eMysql, "ER_CANT_AGGREGATE_2COLLATIONS", INT2NUM(ER_CANT_AGGREGATE_2COLLATIONS));
    rb_define_const(eMysql, "ER_DROP_USER", INT2NUM(ER_DROP_USER));
    rb_define_const(eMysql, "ER_REVOKE_GRANTS", INT2NUM(ER_REVOKE_GRANTS));
    rb_define_const(eMysql, "ER_CANT_AGGREGATE_3COLLATIONS", INT2NUM(ER_CANT_AGGREGATE_3COLLATIONS));
    rb_define_const(eMysql, "ER_CANT_AGGREGATE_NCOLLATIONS", INT2NUM(ER_CANT_AGGREGATE_NCOLLATIONS));
    rb_define_const(eMysql, "ER_VARIABLE_IS_NOT_STRUCT", INT2NUM(ER_VARIABLE_IS_NOT_STRUCT));
    rb_define_const(eMysql, "ER_UNKNOWN_COLLATION", INT2NUM(ER_UNKNOWN_COLLATION));
    rb_define_const(eMysql, "ER_SLAVE_IGNORED_SSL_PARAMS", INT2NUM(ER_SLAVE_IGNORED_SSL_PARAMS));
    rb_define_const(eMysql, "ER_SERVER_IS_IN_SECURE_AUTH_MODE", INT2NUM(ER_SERVER_IS_IN_SECURE_AUTH_MODE));
    rb_define_const(eMysql, "ER_WARN_FIELD_RESOLVED", INT2NUM(ER_WARN_FIELD_RESOLVED));
    rb_define_const(eMysql, "ER_BAD_SLAVE_UNTIL_COND", INT2NUM(ER_BAD_SLAVE_UNTIL_COND));
    rb_define_const(eMysql, "ER_MISSING_SKIP_SLAVE", INT2NUM(ER_MISSING_SKIP_SLAVE));
    rb_define_const(eMysql, "ER_UNTIL_COND_IGNORED", INT2NUM(ER_UNTIL_COND_IGNORED));
    rb_define_const(eMysql, "ER_WRONG_NAME_FOR_INDEX", INT2NUM(ER_WRONG_NAME_FOR_INDEX));
    rb_define_const(eMysql, "ER_WRONG_NAME_FOR_CATALOG", INT2NUM(ER_WRONG_NAME_FOR_CATALOG));
    rb_define_const(eMysql, "ER_WARN_QC_RESIZE", INT2NUM(ER_WARN_QC_RESIZE));
    rb_define_const(eMysql, "ER_BAD_FT_COLUMN", INT2NUM(ER_BAD_FT_COLUMN));
    rb_define_const(eMysql, "ER_UNKNOWN_KEY_CACHE", INT2NUM(ER_UNKNOWN_KEY_CACHE));
    rb_define_const(eMysql, "ER_WARN_HOSTNAME_WONT_WORK", INT2NUM(ER_WARN_HOSTNAME_WONT_WORK));
    rb_define_const(eMysql, "ER_UNKNOWN_STORAGE_ENGINE", INT2NUM(ER_UNKNOWN_STORAGE_ENGINE));
    rb_define_const(eMysql, "ER_WARN_DEPRECATED_SYNTAX", INT2NUM(ER_WARN_DEPRECATED_SYNTAX));
    rb_define_const(eMysql, "ER_NON_UPDATABLE_TABLE", INT2NUM(ER_NON_UPDATABLE_TABLE));
    rb_define_const(eMysql, "ER_FEATURE_DISABLED", INT2NUM(ER_FEATURE_DISABLED));
    rb_define_const(eMysql, "ER_OPTION_PREVENTS_STATEMENT", INT2NUM(ER_OPTION_PREVENTS_STATEMENT));
    rb_define_const(eMysql, "ER_DUPLICATED_VALUE_IN_TYPE", INT2NUM(ER_DUPLICATED_VALUE_IN_TYPE));
    rb_define_const(eMysql, "ER_TRUNCATED_WRONG_VALUE", INT2NUM(ER_TRUNCATED_WRONG_VALUE));
    rb_define_const(eMysql, "ER_TOO_MUCH_AUTO_TIMESTAMP_COLS", INT2NUM(ER_TOO_MUCH_AUTO_TIMESTAMP_COLS));
    rb_define_const(eMysql, "ER_INVALID_ON_UPDATE", INT2NUM(ER_INVALID_ON_UPDATE));
    rb_define_const(eMysql, "ER_UNSUPPORTED_PS", INT2NUM(ER_UNSUPPORTED_PS));
    rb_define_const(eMysql, "ER_GET_ERRMSG", INT2NUM(ER_GET_ERRMSG));
    rb_define_const(eMysql, "ER_GET_TEMPORARY_ERRMSG", INT2NUM(ER_GET_TEMPORARY_ERRMSG));
    rb_define_const(eMysql, "ER_UNKNOWN_TIME_ZONE", INT2NUM(ER_UNKNOWN_TIME_ZONE));
    rb_define_const(eMysql, "ER_WARN_INVALID_TIMESTAMP", INT2NUM(ER_WARN_INVALID_TIMESTAMP));
    rb_define_const(eMysql, "ER_INVALID_CHARACTER_STRING", INT2NUM(ER_INVALID_CHARACTER_STRING));
    rb_define_const(eMysql, "ER_WARN_ALLOWED_PACKET_OVERFLOWED", INT2NUM(ER_WARN_ALLOWED_PACKET_OVERFLOWED));
    rb_define_const(eMysql, "ER_CONFLICTING_DECLARATIONS", INT2NUM(ER_CONFLICTING_DECLARATIONS));
    rb_define_const(eMysql, "ER_SP_NO_RECURSIVE_CREATE", INT2NUM(ER_SP_NO_RECURSIVE_CREATE));
    rb_define_const(eMysql, "ER_SP_ALREADY_EXISTS", INT2NUM(ER_SP_ALREADY_EXISTS));
    rb_define_const(eMysql, "ER_SP_DOES_NOT_EXIST", INT2NUM(ER_SP_DOES_NOT_EXIST));
    rb_define_const(eMysql, "ER_SP_DROP_FAILED", INT2NUM(ER_SP_DROP_FAILED));
    rb_define_const(eMysql, "ER_SP_STORE_FAILED", INT2NUM(ER_SP_STORE_FAILED));
    rb_define_const(eMysql, "ER_SP_LILABEL_MISMATCH", INT2NUM(ER_SP_LILABEL_MISMATCH));
    rb_define_const(eMysql, "ER_SP_LABEL_REDEFINE", INT2NUM(ER_SP_LABEL_REDEFINE));
    rb_define_const(eMysql, "ER_SP_LABEL_MISMATCH", INT2NUM(ER_SP_LABEL_MISMATCH));
    rb_define_const(eMysql, "ER_SP_UNINIT_VAR", INT2NUM(ER_SP_UNINIT_VAR));
    rb_define_const(eMysql, "ER_SP_BADSELECT", INT2NUM(ER_SP_BADSELECT));
    rb_define_const(eMysql, "ER_SP_BADRETURN", INT2NUM(ER_SP_BADRETURN));
    rb_define_const(eMysql, "ER_SP_BADSTATEMENT", INT2NUM(ER_SP_BADSTATEMENT));
    rb_define_const(eMysql, "ER_UPDATE_LOG_DEPRECATED_IGNORED", INT2NUM(ER_UPDATE_LOG_DEPRECATED_IGNORED));
    rb_define_const(eMysql, "ER_UPDATE_LOG_DEPRECATED_TRANSLATED", INT2NUM(ER_UPDATE_LOG_DEPRECATED_TRANSLATED));
    rb_define_const(eMysql, "ER_QUERY_INTERRUPTED", INT2NUM(ER_QUERY_INTERRUPTED));
    rb_define_const(eMysql, "ER_SP_WRONG_NO_OF_ARGS", INT2NUM(ER_SP_WRONG_NO_OF_ARGS));
    rb_define_const(eMysql, "ER_SP_COND_MISMATCH", INT2NUM(ER_SP_COND_MISMATCH));
    rb_define_const(eMysql, "ER_SP_NORETURN", INT2NUM(ER_SP_NORETURN));
    rb_define_const(eMysql, "ER_SP_NORETURNEND", INT2NUM(ER_SP_NORETURNEND));
    rb_define_const(eMysql, "ER_SP_BAD_CURSOR_QUERY", INT2NUM(ER_SP_BAD_CURSOR_QUERY));
    rb_define_const(eMysql, "ER_SP_BAD_CURSOR_SELECT", INT2NUM(ER_SP_BAD_CURSOR_SELECT));
    rb_define_const(eMysql, "ER_SP_CURSOR_MISMATCH", INT2NUM(ER_SP_CURSOR_MISMATCH));
    rb_define_const(eMysql, "ER_SP_CURSOR_ALREADY_OPEN", INT2NUM(ER_SP_CURSOR_ALREADY_OPEN));
    rb_define_const(eMysql, "ER_SP_CURSOR_NOT_OPEN", INT2NUM(ER_SP_CURSOR_NOT_OPEN));
    rb_define_const(eMysql, "ER_SP_UNDECLARED_VAR", INT2NUM(ER_SP_UNDECLARED_VAR));
    rb_define_const(eMysql, "ER_SP_WRONG_NO_OF_FETCH_ARGS", INT2NUM(ER_SP_WRONG_NO_OF_FETCH_ARGS));
    rb_define_const(eMysql, "ER_SP_FETCH_NO_DATA", INT2NUM(ER_SP_FETCH_NO_DATA));
    rb_define_const(eMysql, "ER_SP_DUP_PARAM", INT2NUM(ER_SP_DUP_PARAM));
    rb_define_const(eMysql, "ER_SP_DUP_VAR", INT2NUM(ER_SP_DUP_VAR));
    rb_define_const(eMysql, "ER_SP_DUP_COND", INT2NUM(ER_SP_DUP_COND));
    rb_define_const(eMysql, "ER_SP_DUP_CURS", INT2NUM(ER_SP_DUP_CURS));
    rb_define_const(eMysql, "ER_SP_CANT_ALTER", INT2NUM(ER_SP_CANT_ALTER));
    rb_define_const(eMysql, "ER_SP_SUBSELECT_NYI", INT2NUM(ER_SP_SUBSELECT_NYI));
    rb_define_const(eMysql, "ER_STMT_NOT_ALLOWED_IN_SF_OR_TRG", INT2NUM(ER_STMT_NOT_ALLOWED_IN_SF_OR_TRG));
    rb_define_const(eMysql, "ER_SP_VARCOND_AFTER_CURSHNDLR", INT2NUM(ER_SP_VARCOND_AFTER_CURSHNDLR));
    rb_define_const(eMysql, "ER_SP_CURSOR_AFTER_HANDLER", INT2NUM(ER_SP_CURSOR_AFTER_HANDLER));
    rb_define_const(eMysql, "ER_SP_CASE_NOT_FOUND", INT2NUM(ER_SP_CASE_NOT_FOUND));
    rb_define_const(eMysql, "ER_FPARSER_TOO_BIG_FILE", INT2NUM(ER_FPARSER_TOO_BIG_FILE));
    rb_define_const(eMysql, "ER_FPARSER_BAD_HEADER", INT2NUM(ER_FPARSER_BAD_HEADER));
    rb_define_const(eMysql, "ER_FPARSER_EOF_IN_COMMENT", INT2NUM(ER_FPARSER_EOF_IN_COMMENT));
    rb_define_const(eMysql, "ER_FPARSER_ERROR_IN_PARAMETER", INT2NUM(ER_FPARSER_ERROR_IN_PARAMETER));
    rb_define_const(eMysql, "ER_FPARSER_EOF_IN_UNKNOWN_PARAMETER", INT2NUM(ER_FPARSER_EOF_IN_UNKNOWN_PARAMETER));
    rb_define_const(eMysql, "ER_VIEW_NO_EXPLAIN", INT2NUM(ER_VIEW_NO_EXPLAIN));
    rb_define_const(eMysql, "ER_FRM_UNKNOWN_TYPE", INT2NUM(ER_FRM_UNKNOWN_TYPE));
    rb_define_const(eMysql, "ER_WRONG_OBJECT", INT2NUM(ER_WRONG_OBJECT));
    rb_define_const(eMysql, "ER_NONUPDATEABLE_COLUMN", INT2NUM(ER_NONUPDATEABLE_COLUMN));
    rb_define_const(eMysql, "ER_VIEW_SELECT_DERIVED", INT2NUM(ER_VIEW_SELECT_DERIVED));
    rb_define_const(eMysql, "ER_VIEW_SELECT_CLAUSE", INT2NUM(ER_VIEW_SELECT_CLAUSE));
    rb_define_const(eMysql, "ER_VIEW_SELECT_VARIABLE", INT2NUM(ER_VIEW_SELECT_VARIABLE));
    rb_define_const(eMysql, "ER_VIEW_SELECT_TMPTABLE", INT2NUM(ER_VIEW_SELECT_TMPTABLE));
    rb_define_const(eMysql, "ER_VIEW_WRONG_LIST", INT2NUM(ER_VIEW_WRONG_LIST));
    rb_define_const(eMysql, "ER_WARN_VIEW_MERGE", INT2NUM(ER_WARN_VIEW_MERGE));
    rb_define_const(eMysql, "ER_WARN_VIEW_WITHOUT_KEY", INT2NUM(ER_WARN_VIEW_WITHOUT_KEY));
    rb_define_const(eMysql, "ER_VIEW_INVALID", INT2NUM(ER_VIEW_INVALID));
    rb_define_const(eMysql, "ER_SP_NO_DROP_SP", INT2NUM(ER_SP_NO_DROP_SP));
    rb_define_const(eMysql, "ER_SP_GOTO_IN_HNDLR", INT2NUM(ER_SP_GOTO_IN_HNDLR));
    rb_define_const(eMysql, "ER_TRG_ALREADY_EXISTS", INT2NUM(ER_TRG_ALREADY_EXISTS));
    rb_define_const(eMysql, "ER_TRG_DOES_NOT_EXIST", INT2NUM(ER_TRG_DOES_NOT_EXIST));
    rb_define_const(eMysql, "ER_TRG_ON_VIEW_OR_TEMP_TABLE", INT2NUM(ER_TRG_ON_VIEW_OR_TEMP_TABLE));
    rb_define_const(eMysql, "ER_TRG_CANT_CHANGE_ROW", INT2NUM(ER_TRG_CANT_CHANGE_ROW));
    rb_define_const(eMysql, "ER_TRG_NO_SUCH_ROW_IN_TRG", INT2NUM(ER_TRG_NO_SUCH_ROW_IN_TRG));
    rb_define_const(eMysql, "ER_NO_DEFAULT_FOR_FIELD", INT2NUM(ER_NO_DEFAULT_FOR_FIELD));
    rb_define_const(eMysql, "ER_DIVISION_BY_ZERO", INT2NUM(ER_DIVISION_BY_ZERO));
    rb_define_const(eMysql, "ER_TRUNCATED_WRONG_VALUE_FOR_FIELD", INT2NUM(ER_TRUNCATED_WRONG_VALUE_FOR_FIELD));
    rb_define_const(eMysql, "ER_ILLEGAL_VALUE_FOR_TYPE", INT2NUM(ER_ILLEGAL_VALUE_FOR_TYPE));
    rb_define_const(eMysql, "ER_VIEW_NONUPD_CHECK", INT2NUM(ER_VIEW_NONUPD_CHECK));
    rb_define_const(eMysql, "ER_VIEW_CHECK_FAILED", INT2NUM(ER_VIEW_CHECK_FAILED));
    rb_define_const(eMysql, "ER_PROCACCESS_DENIED_ERROR", INT2NUM(ER_PROCACCESS_DENIED_ERROR));
    rb_define_const(eMysql, "ER_RELAY_LOG_FAIL", INT2NUM(ER_RELAY_LOG_FAIL));
    rb_define_const(eMysql, "ER_PASSWD_LENGTH", INT2NUM(ER_PASSWD_LENGTH));
    rb_define_const(eMysql, "ER_UNKNOWN_TARGET_BINLOG", INT2NUM(ER_UNKNOWN_TARGET_BINLOG));
    rb_define_const(eMysql, "ER_IO_ERR_LOG_INDEX_READ", INT2NUM(ER_IO_ERR_LOG_INDEX_READ));
    rb_define_const(eMysql, "ER_BINLOG_PURGE_PROHIBITED", INT2NUM(ER_BINLOG_PURGE_PROHIBITED));
    rb_define_const(eMysql, "ER_FSEEK_FAIL", INT2NUM(ER_FSEEK_FAIL));
    rb_define_const(eMysql, "ER_BINLOG_PURGE_FATAL_ERR", INT2NUM(ER_BINLOG_PURGE_FATAL_ERR));
    rb_define_const(eMysql, "ER_LOG_IN_USE", INT2NUM(ER_LOG_IN_USE));
    rb_define_const(eMysql, "ER_LOG_PURGE_UNKNOWN_ERR", INT2NUM(ER_LOG_PURGE_UNKNOWN_ERR));
    rb_define_const(eMysql, "ER_RELAY_LOG_INIT", INT2NUM(ER_RELAY_LOG_INIT));
    rb_define_const(eMysql, "ER_NO_BINARY_LOGGING", INT2NUM(ER_NO_BINARY_LOGGING));
    rb_define_const(eMysql, "ER_RESERVED_SYNTAX", INT2NUM(ER_RESERVED_SYNTAX));
    rb_define_const(eMysql, "ER_WSAS_FAILED", INT2NUM(ER_WSAS_FAILED));
    rb_define_const(eMysql, "ER_DIFF_GROUPS_PROC", INT2NUM(ER_DIFF_GROUPS_PROC));
    rb_define_const(eMysql, "ER_NO_GROUP_FOR_PROC", INT2NUM(ER_NO_GROUP_FOR_PROC));
    rb_define_const(eMysql, "ER_ORDER_WITH_PROC", INT2NUM(ER_ORDER_WITH_PROC));
    rb_define_const(eMysql, "ER_LOGGING_PROHIBIT_CHANGING_OF", INT2NUM(ER_LOGGING_PROHIBIT_CHANGING_OF));
    rb_define_const(eMysql, "ER_NO_FILE_MAPPING", INT2NUM(ER_NO_FILE_MAPPING));
    rb_define_const(eMysql, "ER_WRONG_MAGIC", INT2NUM(ER_WRONG_MAGIC));
    rb_define_const(eMysql, "ER_PS_MANY_PARAM", INT2NUM(ER_PS_MANY_PARAM));
    rb_define_const(eMysql, "ER_KEY_PART_0", INT2NUM(ER_KEY_PART_0));
    rb_define_const(eMysql, "ER_VIEW_CHECKSUM", INT2NUM(ER_VIEW_CHECKSUM));
    rb_define_const(eMysql, "ER_VIEW_MULTIUPDATE", INT2NUM(ER_VIEW_MULTIUPDATE));
    rb_define_const(eMysql, "ER_VIEW_NO_INSERT_FIELD_LIST", INT2NUM(ER_VIEW_NO_INSERT_FIELD_LIST));
    rb_define_const(eMysql, "ER_VIEW_DELETE_MERGE_VIEW", INT2NUM(ER_VIEW_DELETE_MERGE_VIEW));
    rb_define_const(eMysql, "ER_CANNOT_USER", INT2NUM(ER_CANNOT_USER));
    rb_define_const(eMysql, "ER_XAER_NOTA", INT2NUM(ER_XAER_NOTA));
    rb_define_const(eMysql, "ER_XAER_INVAL", INT2NUM(ER_XAER_INVAL));
    rb_define_const(eMysql, "ER_XAER_RMFAIL", INT2NUM(ER_XAER_RMFAIL));
    rb_define_const(eMysql, "ER_XAER_OUTSIDE", INT2NUM(ER_XAER_OUTSIDE));
    rb_define_const(eMysql, "ER_XAER_RMERR", INT2NUM(ER_XAER_RMERR));
    rb_define_const(eMysql, "ER_XA_RBROLLBACK", INT2NUM(ER_XA_RBROLLBACK));
    rb_define_const(eMysql, "ER_NONEXISTING_PROC_GRANT", INT2NUM(ER_NONEXISTING_PROC_GRANT));
    rb_define_const(eMysql, "ER_PROC_AUTO_GRANT_FAIL", INT2NUM(ER_PROC_AUTO_GRANT_FAIL));
    rb_define_const(eMysql, "ER_PROC_AUTO_REVOKE_FAIL", INT2NUM(ER_PROC_AUTO_REVOKE_FAIL));
    rb_define_const(eMysql, "ER_DATA_TOO_LONG", INT2NUM(ER_DATA_TOO_LONG));
    rb_define_const(eMysql, "ER_SP_BAD_SQLSTATE", INT2NUM(ER_SP_BAD_SQLSTATE));
    rb_define_const(eMysql, "ER_STARTUP", INT2NUM(ER_STARTUP));
    rb_define_const(eMysql, "ER_LOAD_FROM_FIXED_SIZE_ROWS_TO_VAR", INT2NUM(ER_LOAD_FROM_FIXED_SIZE_ROWS_TO_VAR));
    rb_define_const(eMysql, "ER_CANT_CREATE_USER_WITH_GRANT", INT2NUM(ER_CANT_CREATE_USER_WITH_GRANT));
    rb_define_const(eMysql, "ER_WRONG_VALUE_FOR_TYPE", INT2NUM(ER_WRONG_VALUE_FOR_TYPE));
    rb_define_const(eMysql, "ER_TABLE_DEF_CHANGED", INT2NUM(ER_TABLE_DEF_CHANGED));
    rb_define_const(eMysql, "ER_SP_DUP_HANDLER", INT2NUM(ER_SP_DUP_HANDLER));
    rb_define_const(eMysql, "ER_SP_NOT_VAR_ARG", INT2NUM(ER_SP_NOT_VAR_ARG));
    rb_define_const(eMysql, "ER_SP_NO_RETSET", INT2NUM(ER_SP_NO_RETSET));
    rb_define_const(eMysql, "ER_CANT_CREATE_GEOMETRY_OBJECT", INT2NUM(ER_CANT_CREATE_GEOMETRY_OBJECT));
    rb_define_const(eMysql, "ER_FAILED_ROUTINE_BREAK_BINLOG", INT2NUM(ER_FAILED_ROUTINE_BREAK_BINLOG));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_ROUTINE", INT2NUM(ER_BINLOG_UNSAFE_ROUTINE));
    rb_define_const(eMysql, "ER_BINLOG_CREATE_ROUTINE_NEED_SUPER", INT2NUM(ER_BINLOG_CREATE_ROUTINE_NEED_SUPER));
    rb_define_const(eMysql, "ER_EXEC_STMT_WITH_OPEN_CURSOR", INT2NUM(ER_EXEC_STMT_WITH_OPEN_CURSOR));
    rb_define_const(eMysql, "ER_STMT_HAS_NO_OPEN_CURSOR", INT2NUM(ER_STMT_HAS_NO_OPEN_CURSOR));
    rb_define_const(eMysql, "ER_COMMIT_NOT_ALLOWED_IN_SF_OR_TRG", INT2NUM(ER_COMMIT_NOT_ALLOWED_IN_SF_OR_TRG));
    rb_define_const(eMysql, "ER_NO_DEFAULT_FOR_VIEW_FIELD", INT2NUM(ER_NO_DEFAULT_FOR_VIEW_FIELD));
    rb_define_const(eMysql, "ER_SP_NO_RECURSION", INT2NUM(ER_SP_NO_RECURSION));
    rb_define_const(eMysql, "ER_TOO_BIG_SCALE", INT2NUM(ER_TOO_BIG_SCALE));
    rb_define_const(eMysql, "ER_TOO_BIG_PRECISION", INT2NUM(ER_TOO_BIG_PRECISION));
    rb_define_const(eMysql, "ER_M_BIGGER_THAN_D", INT2NUM(ER_M_BIGGER_THAN_D));
    rb_define_const(eMysql, "ER_WRONG_LOCK_OF_SYSTEM_TABLE", INT2NUM(ER_WRONG_LOCK_OF_SYSTEM_TABLE));
    rb_define_const(eMysql, "ER_CONNECT_TO_FOREIGN_DATA_SOURCE", INT2NUM(ER_CONNECT_TO_FOREIGN_DATA_SOURCE));
    rb_define_const(eMysql, "ER_QUERY_ON_FOREIGN_DATA_SOURCE", INT2NUM(ER_QUERY_ON_FOREIGN_DATA_SOURCE));
    rb_define_const(eMysql, "ER_FOREIGN_DATA_SOURCE_DOESNT_EXIST", INT2NUM(ER_FOREIGN_DATA_SOURCE_DOESNT_EXIST));
    rb_define_const(eMysql, "ER_FOREIGN_DATA_STRING_INVALID_CANT_CREATE", INT2NUM(ER_FOREIGN_DATA_STRING_INVALID_CANT_CREATE));
    rb_define_const(eMysql, "ER_FOREIGN_DATA_STRING_INVALID", INT2NUM(ER_FOREIGN_DATA_STRING_INVALID));
    rb_define_const(eMysql, "ER_CANT_CREATE_FEDERATED_TABLE", INT2NUM(ER_CANT_CREATE_FEDERATED_TABLE));
    rb_define_const(eMysql, "ER_TRG_IN_WRONG_SCHEMA", INT2NUM(ER_TRG_IN_WRONG_SCHEMA));
    rb_define_const(eMysql, "ER_STACK_OVERRUN_NEED_MORE", INT2NUM(ER_STACK_OVERRUN_NEED_MORE));
    rb_define_const(eMysql, "ER_TOO_LONG_BODY", INT2NUM(ER_TOO_LONG_BODY));
    rb_define_const(eMysql, "ER_WARN_CANT_DROP_DEFAULT_KEYCACHE", INT2NUM(ER_WARN_CANT_DROP_DEFAULT_KEYCACHE));
    rb_define_const(eMysql, "ER_TOO_BIG_DISPLAYWIDTH", INT2NUM(ER_TOO_BIG_DISPLAYWIDTH));
    rb_define_const(eMysql, "ER_XAER_DUPID", INT2NUM(ER_XAER_DUPID));
    rb_define_const(eMysql, "ER_DATETIME_FUNCTION_OVERFLOW", INT2NUM(ER_DATETIME_FUNCTION_OVERFLOW));
    rb_define_const(eMysql, "ER_CANT_UPDATE_USED_TABLE_IN_SF_OR_TRG", INT2NUM(ER_CANT_UPDATE_USED_TABLE_IN_SF_OR_TRG));
    rb_define_const(eMysql, "ER_VIEW_PREVENT_UPDATE", INT2NUM(ER_VIEW_PREVENT_UPDATE));
    rb_define_const(eMysql, "ER_PS_NO_RECURSION", INT2NUM(ER_PS_NO_RECURSION));
    rb_define_const(eMysql, "ER_SP_CANT_SET_AUTOCOMMIT", INT2NUM(ER_SP_CANT_SET_AUTOCOMMIT));
    rb_define_const(eMysql, "ER_MALFORMED_DEFINER", INT2NUM(ER_MALFORMED_DEFINER));
    rb_define_const(eMysql, "ER_VIEW_FRM_NO_USER", INT2NUM(ER_VIEW_FRM_NO_USER));
    rb_define_const(eMysql, "ER_VIEW_OTHER_USER", INT2NUM(ER_VIEW_OTHER_USER));
    rb_define_const(eMysql, "ER_NO_SUCH_USER", INT2NUM(ER_NO_SUCH_USER));
    rb_define_const(eMysql, "ER_FORBID_SCHEMA_CHANGE", INT2NUM(ER_FORBID_SCHEMA_CHANGE));
    rb_define_const(eMysql, "ER_ROW_IS_REFERENCED_2", INT2NUM(ER_ROW_IS_REFERENCED_2));
    rb_define_const(eMysql, "ER_NO_REFERENCED_ROW_2", INT2NUM(ER_NO_REFERENCED_ROW_2));
    rb_define_const(eMysql, "ER_SP_BAD_VAR_SHADOW", INT2NUM(ER_SP_BAD_VAR_SHADOW));
    rb_define_const(eMysql, "ER_TRG_NO_DEFINER", INT2NUM(ER_TRG_NO_DEFINER));
    rb_define_const(eMysql, "ER_OLD_FILE_FORMAT", INT2NUM(ER_OLD_FILE_FORMAT));
    rb_define_const(eMysql, "ER_SP_RECURSION_LIMIT", INT2NUM(ER_SP_RECURSION_LIMIT));
    rb_define_const(eMysql, "ER_SP_PROC_TABLE_CORRUPT", INT2NUM(ER_SP_PROC_TABLE_CORRUPT));
    rb_define_const(eMysql, "ER_SP_WRONG_NAME", INT2NUM(ER_SP_WRONG_NAME));
    rb_define_const(eMysql, "ER_TABLE_NEEDS_UPGRADE", INT2NUM(ER_TABLE_NEEDS_UPGRADE));
    rb_define_const(eMysql, "ER_SP_NO_AGGREGATE", INT2NUM(ER_SP_NO_AGGREGATE));
    rb_define_const(eMysql, "ER_MAX_PREPARED_STMT_COUNT_REACHED", INT2NUM(ER_MAX_PREPARED_STMT_COUNT_REACHED));
    rb_define_const(eMysql, "ER_VIEW_RECURSIVE", INT2NUM(ER_VIEW_RECURSIVE));
    rb_define_const(eMysql, "ER_NON_GROUPING_FIELD_USED", INT2NUM(ER_NON_GROUPING_FIELD_USED));
    rb_define_const(eMysql, "ER_TABLE_CANT_HANDLE_SPKEYS", INT2NUM(ER_TABLE_CANT_HANDLE_SPKEYS));
    rb_define_const(eMysql, "ER_NO_TRIGGERS_ON_SYSTEM_SCHEMA", INT2NUM(ER_NO_TRIGGERS_ON_SYSTEM_SCHEMA));
    rb_define_const(eMysql, "ER_REMOVED_SPACES", INT2NUM(ER_REMOVED_SPACES));
    rb_define_const(eMysql, "ER_AUTOINC_READ_FAILED", INT2NUM(ER_AUTOINC_READ_FAILED));
    rb_define_const(eMysql, "ER_USERNAME", INT2NUM(ER_USERNAME));
    rb_define_const(eMysql, "ER_HOSTNAME", INT2NUM(ER_HOSTNAME));
    rb_define_const(eMysql, "ER_WRONG_STRING_LENGTH", INT2NUM(ER_WRONG_STRING_LENGTH));
    rb_define_const(eMysql, "ER_NON_INSERTABLE_TABLE", INT2NUM(ER_NON_INSERTABLE_TABLE));
    rb_define_const(eMysql, "ER_ADMIN_WRONG_MRG_TABLE", INT2NUM(ER_ADMIN_WRONG_MRG_TABLE));
    rb_define_const(eMysql, "ER_TOO_HIGH_LEVEL_OF_NESTING_FOR_SELECT", INT2NUM(ER_TOO_HIGH_LEVEL_OF_NESTING_FOR_SELECT));
    rb_define_const(eMysql, "ER_NAME_BECOMES_EMPTY", INT2NUM(ER_NAME_BECOMES_EMPTY));
    rb_define_const(eMysql, "ER_AMBIGUOUS_FIELD_TERM", INT2NUM(ER_AMBIGUOUS_FIELD_TERM));
    rb_define_const(eMysql, "ER_FOREIGN_SERVER_EXISTS", INT2NUM(ER_FOREIGN_SERVER_EXISTS));
    rb_define_const(eMysql, "ER_FOREIGN_SERVER_DOESNT_EXIST", INT2NUM(ER_FOREIGN_SERVER_DOESNT_EXIST));
    rb_define_const(eMysql, "ER_ILLEGAL_HA_CREATE_OPTION", INT2NUM(ER_ILLEGAL_HA_CREATE_OPTION));
    rb_define_const(eMysql, "ER_PARTITION_REQUIRES_VALUES_ERROR", INT2NUM(ER_PARTITION_REQUIRES_VALUES_ERROR));
    rb_define_const(eMysql, "ER_PARTITION_WRONG_VALUES_ERROR", INT2NUM(ER_PARTITION_WRONG_VALUES_ERROR));
    rb_define_const(eMysql, "ER_PARTITION_MAXVALUE_ERROR", INT2NUM(ER_PARTITION_MAXVALUE_ERROR));
    rb_define_const(eMysql, "ER_PARTITION_SUBPARTITION_ERROR", INT2NUM(ER_PARTITION_SUBPARTITION_ERROR));
    rb_define_const(eMysql, "ER_PARTITION_SUBPART_MIX_ERROR", INT2NUM(ER_PARTITION_SUBPART_MIX_ERROR));
    rb_define_const(eMysql, "ER_PARTITION_WRONG_NO_PART_ERROR", INT2NUM(ER_PARTITION_WRONG_NO_PART_ERROR));
    rb_define_const(eMysql, "ER_PARTITION_WRONG_NO_SUBPART_ERROR", INT2NUM(ER_PARTITION_WRONG_NO_SUBPART_ERROR));
    rb_define_const(eMysql, "ER_WRONG_EXPR_IN_PARTITION_FUNC_ERROR", INT2NUM(ER_WRONG_EXPR_IN_PARTITION_FUNC_ERROR));
    rb_define_const(eMysql, "ER_NO_CONST_EXPR_IN_RANGE_OR_LIST_ERROR", INT2NUM(ER_NO_CONST_EXPR_IN_RANGE_OR_LIST_ERROR));
    rb_define_const(eMysql, "ER_FIELD_NOT_FOUND_PART_ERROR", INT2NUM(ER_FIELD_NOT_FOUND_PART_ERROR));
    rb_define_const(eMysql, "ER_LIST_OF_FIELDS_ONLY_IN_HASH_ERROR", INT2NUM(ER_LIST_OF_FIELDS_ONLY_IN_HASH_ERROR));
    rb_define_const(eMysql, "ER_INCONSISTENT_PARTITION_INFO_ERROR", INT2NUM(ER_INCONSISTENT_PARTITION_INFO_ERROR));
    rb_define_const(eMysql, "ER_PARTITION_FUNC_NOT_ALLOWED_ERROR", INT2NUM(ER_PARTITION_FUNC_NOT_ALLOWED_ERROR));
    rb_define_const(eMysql, "ER_PARTITIONS_MUST_BE_DEFINED_ERROR", INT2NUM(ER_PARTITIONS_MUST_BE_DEFINED_ERROR));
    rb_define_const(eMysql, "ER_RANGE_NOT_INCREASING_ERROR", INT2NUM(ER_RANGE_NOT_INCREASING_ERROR));
    rb_define_const(eMysql, "ER_INCONSISTENT_TYPE_OF_FUNCTIONS_ERROR", INT2NUM(ER_INCONSISTENT_TYPE_OF_FUNCTIONS_ERROR));
    rb_define_const(eMysql, "ER_MULTIPLE_DEF_CONST_IN_LIST_PART_ERROR", INT2NUM(ER_MULTIPLE_DEF_CONST_IN_LIST_PART_ERROR));
    rb_define_const(eMysql, "ER_PARTITION_ENTRY_ERROR", INT2NUM(ER_PARTITION_ENTRY_ERROR));
    rb_define_const(eMysql, "ER_MIX_HANDLER_ERROR", INT2NUM(ER_MIX_HANDLER_ERROR));
    rb_define_const(eMysql, "ER_PARTITION_NOT_DEFINED_ERROR", INT2NUM(ER_PARTITION_NOT_DEFINED_ERROR));
    rb_define_const(eMysql, "ER_TOO_MANY_PARTITIONS_ERROR", INT2NUM(ER_TOO_MANY_PARTITIONS_ERROR));
    rb_define_const(eMysql, "ER_SUBPARTITION_ERROR", INT2NUM(ER_SUBPARTITION_ERROR));
    rb_define_const(eMysql, "ER_CANT_CREATE_HANDLER_FILE", INT2NUM(ER_CANT_CREATE_HANDLER_FILE));
    rb_define_const(eMysql, "ER_BLOB_FIELD_IN_PART_FUNC_ERROR", INT2NUM(ER_BLOB_FIELD_IN_PART_FUNC_ERROR));
    rb_define_const(eMysql, "ER_UNIQUE_KEY_NEED_ALL_FIELDS_IN_PF", INT2NUM(ER_UNIQUE_KEY_NEED_ALL_FIELDS_IN_PF));
    rb_define_const(eMysql, "ER_NO_PARTS_ERROR", INT2NUM(ER_NO_PARTS_ERROR));
    rb_define_const(eMysql, "ER_PARTITION_MGMT_ON_NONPARTITIONED", INT2NUM(ER_PARTITION_MGMT_ON_NONPARTITIONED));
    rb_define_const(eMysql, "ER_FOREIGN_KEY_ON_PARTITIONED", INT2NUM(ER_FOREIGN_KEY_ON_PARTITIONED));
    rb_define_const(eMysql, "ER_DROP_PARTITION_NON_EXISTENT", INT2NUM(ER_DROP_PARTITION_NON_EXISTENT));
    rb_define_const(eMysql, "ER_DROP_LAST_PARTITION", INT2NUM(ER_DROP_LAST_PARTITION));
    rb_define_const(eMysql, "ER_COALESCE_ONLY_ON_HASH_PARTITION", INT2NUM(ER_COALESCE_ONLY_ON_HASH_PARTITION));
    rb_define_const(eMysql, "ER_REORG_HASH_ONLY_ON_SAME_NO", INT2NUM(ER_REORG_HASH_ONLY_ON_SAME_NO));
    rb_define_const(eMysql, "ER_REORG_NO_PARAM_ERROR", INT2NUM(ER_REORG_NO_PARAM_ERROR));
    rb_define_const(eMysql, "ER_ONLY_ON_RANGE_LIST_PARTITION", INT2NUM(ER_ONLY_ON_RANGE_LIST_PARTITION));
    rb_define_const(eMysql, "ER_ADD_PARTITION_SUBPART_ERROR", INT2NUM(ER_ADD_PARTITION_SUBPART_ERROR));
    rb_define_const(eMysql, "ER_ADD_PARTITION_NO_NEW_PARTITION", INT2NUM(ER_ADD_PARTITION_NO_NEW_PARTITION));
    rb_define_const(eMysql, "ER_COALESCE_PARTITION_NO_PARTITION", INT2NUM(ER_COALESCE_PARTITION_NO_PARTITION));
    rb_define_const(eMysql, "ER_REORG_PARTITION_NOT_EXIST", INT2NUM(ER_REORG_PARTITION_NOT_EXIST));
    rb_define_const(eMysql, "ER_SAME_NAME_PARTITION", INT2NUM(ER_SAME_NAME_PARTITION));
    rb_define_const(eMysql, "ER_NO_BINLOG_ERROR", INT2NUM(ER_NO_BINLOG_ERROR));
    rb_define_const(eMysql, "ER_CONSECUTIVE_REORG_PARTITIONS", INT2NUM(ER_CONSECUTIVE_REORG_PARTITIONS));
    rb_define_const(eMysql, "ER_REORG_OUTSIDE_RANGE", INT2NUM(ER_REORG_OUTSIDE_RANGE));
    rb_define_const(eMysql, "ER_PARTITION_FUNCTION_FAILURE", INT2NUM(ER_PARTITION_FUNCTION_FAILURE));
    rb_define_const(eMysql, "ER_PART_STATE_ERROR", INT2NUM(ER_PART_STATE_ERROR));
    rb_define_const(eMysql, "ER_LIMITED_PART_RANGE", INT2NUM(ER_LIMITED_PART_RANGE));
    rb_define_const(eMysql, "ER_PLUGIN_IS_NOT_LOADED", INT2NUM(ER_PLUGIN_IS_NOT_LOADED));
    rb_define_const(eMysql, "ER_WRONG_VALUE", INT2NUM(ER_WRONG_VALUE));
    rb_define_const(eMysql, "ER_NO_PARTITION_FOR_GIVEN_VALUE", INT2NUM(ER_NO_PARTITION_FOR_GIVEN_VALUE));
    rb_define_const(eMysql, "ER_FILEGROUP_OPTION_ONLY_ONCE", INT2NUM(ER_FILEGROUP_OPTION_ONLY_ONCE));
    rb_define_const(eMysql, "ER_CREATE_FILEGROUP_FAILED", INT2NUM(ER_CREATE_FILEGROUP_FAILED));
    rb_define_const(eMysql, "ER_DROP_FILEGROUP_FAILED", INT2NUM(ER_DROP_FILEGROUP_FAILED));
    rb_define_const(eMysql, "ER_TABLESPACE_AUTO_EXTEND_ERROR", INT2NUM(ER_TABLESPACE_AUTO_EXTEND_ERROR));
    rb_define_const(eMysql, "ER_WRONG_SIZE_NUMBER", INT2NUM(ER_WRONG_SIZE_NUMBER));
    rb_define_const(eMysql, "ER_SIZE_OVERFLOW_ERROR", INT2NUM(ER_SIZE_OVERFLOW_ERROR));
    rb_define_const(eMysql, "ER_ALTER_FILEGROUP_FAILED", INT2NUM(ER_ALTER_FILEGROUP_FAILED));
    rb_define_const(eMysql, "ER_BINLOG_ROW_LOGGING_FAILED", INT2NUM(ER_BINLOG_ROW_LOGGING_FAILED));
    rb_define_const(eMysql, "ER_BINLOG_ROW_WRONG_TABLE_DEF", INT2NUM(ER_BINLOG_ROW_WRONG_TABLE_DEF));
    rb_define_const(eMysql, "ER_BINLOG_ROW_RBR_TO_SBR", INT2NUM(ER_BINLOG_ROW_RBR_TO_SBR));
    rb_define_const(eMysql, "ER_EVENT_ALREADY_EXISTS", INT2NUM(ER_EVENT_ALREADY_EXISTS));
    rb_define_const(eMysql, "ER_EVENT_STORE_FAILED", INT2NUM(ER_EVENT_STORE_FAILED));
    rb_define_const(eMysql, "ER_EVENT_DOES_NOT_EXIST", INT2NUM(ER_EVENT_DOES_NOT_EXIST));
    rb_define_const(eMysql, "ER_EVENT_CANT_ALTER", INT2NUM(ER_EVENT_CANT_ALTER));
    rb_define_const(eMysql, "ER_EVENT_DROP_FAILED", INT2NUM(ER_EVENT_DROP_FAILED));
    rb_define_const(eMysql, "ER_EVENT_INTERVAL_NOT_POSITIVE_OR_TOO_BIG", INT2NUM(ER_EVENT_INTERVAL_NOT_POSITIVE_OR_TOO_BIG));
    rb_define_const(eMysql, "ER_EVENT_ENDS_BEFORE_STARTS", INT2NUM(ER_EVENT_ENDS_BEFORE_STARTS));
    rb_define_const(eMysql, "ER_EVENT_EXEC_TIME_IN_THE_PAST", INT2NUM(ER_EVENT_EXEC_TIME_IN_THE_PAST));
    rb_define_const(eMysql, "ER_EVENT_OPEN_TABLE_FAILED", INT2NUM(ER_EVENT_OPEN_TABLE_FAILED));
    rb_define_const(eMysql, "ER_EVENT_NEITHER_M_EXPR_NOR_M_AT", INT2NUM(ER_EVENT_NEITHER_M_EXPR_NOR_M_AT));
    rb_define_const(eMysql, "ER_COL_COUNT_DOESNT_MATCH_CORRUPTED", INT2NUM(ER_COL_COUNT_DOESNT_MATCH_CORRUPTED));
    rb_define_const(eMysql, "ER_CANNOT_LOAD_FROM_TABLE", INT2NUM(ER_CANNOT_LOAD_FROM_TABLE));
    rb_define_const(eMysql, "ER_EVENT_CANNOT_DELETE", INT2NUM(ER_EVENT_CANNOT_DELETE));
    rb_define_const(eMysql, "ER_EVENT_COMPILE_ERROR", INT2NUM(ER_EVENT_COMPILE_ERROR));
    rb_define_const(eMysql, "ER_EVENT_SAME_NAME", INT2NUM(ER_EVENT_SAME_NAME));
    rb_define_const(eMysql, "ER_EVENT_DATA_TOO_LONG", INT2NUM(ER_EVENT_DATA_TOO_LONG));
    rb_define_const(eMysql, "ER_DROP_INDEX_FK", INT2NUM(ER_DROP_INDEX_FK));
    rb_define_const(eMysql, "ER_WARN_DEPRECATED_SYNTAX_WITH_VER", INT2NUM(ER_WARN_DEPRECATED_SYNTAX_WITH_VER));
    rb_define_const(eMysql, "ER_CANT_WRITE_LOCK_LOG_TABLE", INT2NUM(ER_CANT_WRITE_LOCK_LOG_TABLE));
    rb_define_const(eMysql, "ER_CANT_LOCK_LOG_TABLE", INT2NUM(ER_CANT_LOCK_LOG_TABLE));
    rb_define_const(eMysql, "ER_FOREIGN_DUPLICATE_KEY", INT2NUM(ER_FOREIGN_DUPLICATE_KEY));
    rb_define_const(eMysql, "ER_COL_COUNT_DOESNT_MATCH_PLEASE_UPDATE", INT2NUM(ER_COL_COUNT_DOESNT_MATCH_PLEASE_UPDATE));
    rb_define_const(eMysql, "ER_TEMP_TABLE_PREVENTS_SWITCH_OUT_OF_RBR", INT2NUM(ER_TEMP_TABLE_PREVENTS_SWITCH_OUT_OF_RBR));
    rb_define_const(eMysql, "ER_STORED_FUNCTION_PREVENTS_SWITCH_BINLOG_FORMAT", INT2NUM(ER_STORED_FUNCTION_PREVENTS_SWITCH_BINLOG_FORMAT));
    rb_define_const(eMysql, "ER_NDB_CANT_SWITCH_BINLOG_FORMAT", INT2NUM(ER_NDB_CANT_SWITCH_BINLOG_FORMAT));
    rb_define_const(eMysql, "ER_PARTITION_NO_TEMPORARY", INT2NUM(ER_PARTITION_NO_TEMPORARY));
    rb_define_const(eMysql, "ER_PARTITION_CONST_DOMAIN_ERROR", INT2NUM(ER_PARTITION_CONST_DOMAIN_ERROR));
    rb_define_const(eMysql, "ER_PARTITION_FUNCTION_IS_NOT_ALLOWED", INT2NUM(ER_PARTITION_FUNCTION_IS_NOT_ALLOWED));
    rb_define_const(eMysql, "ER_DDL_LOG_ERROR", INT2NUM(ER_DDL_LOG_ERROR));
    rb_define_const(eMysql, "ER_NULL_IN_VALUES_LESS_THAN", INT2NUM(ER_NULL_IN_VALUES_LESS_THAN));
    rb_define_const(eMysql, "ER_WRONG_PARTITION_NAME", INT2NUM(ER_WRONG_PARTITION_NAME));
    rb_define_const(eMysql, "ER_CANT_CHANGE_TX_ISOLATION", INT2NUM(ER_CANT_CHANGE_TX_ISOLATION));
    rb_define_const(eMysql, "ER_DUP_ENTRY_AUTOINCREMENT_CASE", INT2NUM(ER_DUP_ENTRY_AUTOINCREMENT_CASE));
    rb_define_const(eMysql, "ER_EVENT_MODIFY_QUEUE_ERROR", INT2NUM(ER_EVENT_MODIFY_QUEUE_ERROR));
    rb_define_const(eMysql, "ER_EVENT_SET_VAR_ERROR", INT2NUM(ER_EVENT_SET_VAR_ERROR));
    rb_define_const(eMysql, "ER_PARTITION_MERGE_ERROR", INT2NUM(ER_PARTITION_MERGE_ERROR));
    rb_define_const(eMysql, "ER_CANT_ACTIVATE_LOG", INT2NUM(ER_CANT_ACTIVATE_LOG));
    rb_define_const(eMysql, "ER_RBR_NOT_AVAILABLE", INT2NUM(ER_RBR_NOT_AVAILABLE));
    rb_define_const(eMysql, "ER_BASE64_DECODE_ERROR", INT2NUM(ER_BASE64_DECODE_ERROR));
    rb_define_const(eMysql, "ER_EVENT_RECURSION_FORBIDDEN", INT2NUM(ER_EVENT_RECURSION_FORBIDDEN));
    rb_define_const(eMysql, "ER_EVENTS_DB_ERROR", INT2NUM(ER_EVENTS_DB_ERROR));
    rb_define_const(eMysql, "ER_ONLY_INTEGERS_ALLOWED", INT2NUM(ER_ONLY_INTEGERS_ALLOWED));
    rb_define_const(eMysql, "ER_UNSUPORTED_LOG_ENGINE", INT2NUM(ER_UNSUPORTED_LOG_ENGINE));
    rb_define_const(eMysql, "ER_BAD_LOG_STATEMENT", INT2NUM(ER_BAD_LOG_STATEMENT));
    rb_define_const(eMysql, "ER_CANT_RENAME_LOG_TABLE", INT2NUM(ER_CANT_RENAME_LOG_TABLE));
    rb_define_const(eMysql, "ER_WRONG_PARAMCOUNT_TO_NATIVE_FCT", INT2NUM(ER_WRONG_PARAMCOUNT_TO_NATIVE_FCT));
    rb_define_const(eMysql, "ER_WRONG_PARAMETERS_TO_NATIVE_FCT", INT2NUM(ER_WRONG_PARAMETERS_TO_NATIVE_FCT));
    rb_define_const(eMysql, "ER_WRONG_PARAMETERS_TO_STORED_FCT", INT2NUM(ER_WRONG_PARAMETERS_TO_STORED_FCT));
    rb_define_const(eMysql, "ER_NATIVE_FCT_NAME_COLLISION", INT2NUM(ER_NATIVE_FCT_NAME_COLLISION));
    rb_define_const(eMysql, "ER_DUP_ENTRY_WITH_KEY_NAME", INT2NUM(ER_DUP_ENTRY_WITH_KEY_NAME));
    rb_define_const(eMysql, "ER_BINLOG_PURGE_EMFILE", INT2NUM(ER_BINLOG_PURGE_EMFILE));
    rb_define_const(eMysql, "ER_EVENT_CANNOT_CREATE_IN_THE_PAST", INT2NUM(ER_EVENT_CANNOT_CREATE_IN_THE_PAST));
    rb_define_const(eMysql, "ER_EVENT_CANNOT_ALTER_IN_THE_PAST", INT2NUM(ER_EVENT_CANNOT_ALTER_IN_THE_PAST));
    rb_define_const(eMysql, "ER_SLAVE_INCIDENT", INT2NUM(ER_SLAVE_INCIDENT));
    rb_define_const(eMysql, "ER_NO_PARTITION_FOR_GIVEN_VALUE_SILENT", INT2NUM(ER_NO_PARTITION_FOR_GIVEN_VALUE_SILENT));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_STATEMENT", INT2NUM(ER_BINLOG_UNSAFE_STATEMENT));
    rb_define_const(eMysql, "ER_SLAVE_FATAL_ERROR", INT2NUM(ER_SLAVE_FATAL_ERROR));
    rb_define_const(eMysql, "ER_SLAVE_RELAY_LOG_READ_FAILURE", INT2NUM(ER_SLAVE_RELAY_LOG_READ_FAILURE));
    rb_define_const(eMysql, "ER_SLAVE_RELAY_LOG_WRITE_FAILURE", INT2NUM(ER_SLAVE_RELAY_LOG_WRITE_FAILURE));
    rb_define_const(eMysql, "ER_SLAVE_CREATE_EVENT_FAILURE", INT2NUM(ER_SLAVE_CREATE_EVENT_FAILURE));
    rb_define_const(eMysql, "ER_SLAVE_MASTER_COM_FAILURE", INT2NUM(ER_SLAVE_MASTER_COM_FAILURE));
    rb_define_const(eMysql, "ER_BINLOG_LOGGING_IMPOSSIBLE", INT2NUM(ER_BINLOG_LOGGING_IMPOSSIBLE));
    rb_define_const(eMysql, "ER_VIEW_NO_CREATION_CTX", INT2NUM(ER_VIEW_NO_CREATION_CTX));
    rb_define_const(eMysql, "ER_VIEW_INVALID_CREATION_CTX", INT2NUM(ER_VIEW_INVALID_CREATION_CTX));
    rb_define_const(eMysql, "ER_SR_INVALID_CREATION_CTX", INT2NUM(ER_SR_INVALID_CREATION_CTX));
    rb_define_const(eMysql, "ER_TRG_CORRUPTED_FILE", INT2NUM(ER_TRG_CORRUPTED_FILE));
    rb_define_const(eMysql, "ER_TRG_NO_CREATION_CTX", INT2NUM(ER_TRG_NO_CREATION_CTX));
    rb_define_const(eMysql, "ER_TRG_INVALID_CREATION_CTX", INT2NUM(ER_TRG_INVALID_CREATION_CTX));
    rb_define_const(eMysql, "ER_EVENT_INVALID_CREATION_CTX", INT2NUM(ER_EVENT_INVALID_CREATION_CTX));
    rb_define_const(eMysql, "ER_TRG_CANT_OPEN_TABLE", INT2NUM(ER_TRG_CANT_OPEN_TABLE));
    rb_define_const(eMysql, "ER_CANT_CREATE_SROUTINE", INT2NUM(ER_CANT_CREATE_SROUTINE));
    rb_define_const(eMysql, "ER_NEVER_USED", INT2NUM(ER_NEVER_USED));
    rb_define_const(eMysql, "ER_NO_FORMAT_DESCRIPTION_EVENT_BEFORE_BINLOG_STATEMENT", INT2NUM(ER_NO_FORMAT_DESCRIPTION_EVENT_BEFORE_BINLOG_STATEMENT));
    rb_define_const(eMysql, "ER_SLAVE_CORRUPT_EVENT", INT2NUM(ER_SLAVE_CORRUPT_EVENT));
    rb_define_const(eMysql, "ER_LOAD_DATA_INVALID_COLUMN", INT2NUM(ER_LOAD_DATA_INVALID_COLUMN));
    rb_define_const(eMysql, "ER_LOG_PURGE_NO_FILE", INT2NUM(ER_LOG_PURGE_NO_FILE));
    rb_define_const(eMysql, "ER_XA_RBTIMEOUT", INT2NUM(ER_XA_RBTIMEOUT));
    rb_define_const(eMysql, "ER_XA_RBDEADLOCK", INT2NUM(ER_XA_RBDEADLOCK));
    rb_define_const(eMysql, "ER_NEED_REPREPARE", INT2NUM(ER_NEED_REPREPARE));
    rb_define_const(eMysql, "ER_DELAYED_NOT_SUPPORTED", INT2NUM(ER_DELAYED_NOT_SUPPORTED));
    rb_define_const(eMysql, "ER_VARIABLE_IS_READONLY", INT2NUM(ER_VARIABLE_IS_READONLY));
    rb_define_const(eMysql, "ER_WARN_ENGINE_TRANSACTION_ROLLBACK", INT2NUM(ER_WARN_ENGINE_TRANSACTION_ROLLBACK));
    rb_define_const(eMysql, "ER_SLAVE_HEARTBEAT_FAILURE", INT2NUM(ER_SLAVE_HEARTBEAT_FAILURE));
    rb_define_const(eMysql, "ER_SLAVE_HEARTBEAT_VALUE_OUT_OF_RANGE", INT2NUM(ER_SLAVE_HEARTBEAT_VALUE_OUT_OF_RANGE));
    rb_define_const(eMysql, "ER_NDB_REPLICATION_SCHEMA_ERROR", INT2NUM(ER_NDB_REPLICATION_SCHEMA_ERROR));
    rb_define_const(eMysql, "ER_CONFLICT_FN_PARSE_ERROR", INT2NUM(ER_CONFLICT_FN_PARSE_ERROR));
    rb_define_const(eMysql, "ER_EXCEPTIONS_WRITE_ERROR", INT2NUM(ER_EXCEPTIONS_WRITE_ERROR));
    rb_define_const(eMysql, "ER_TOO_LONG_TABLE_COMMENT", INT2NUM(ER_TOO_LONG_TABLE_COMMENT));
    rb_define_const(eMysql, "ER_TOO_LONG_FIELD_COMMENT", INT2NUM(ER_TOO_LONG_FIELD_COMMENT));
    rb_define_const(eMysql, "ER_FUNC_INEXISTENT_NAME_COLLISION", INT2NUM(ER_FUNC_INEXISTENT_NAME_COLLISION));
    rb_define_const(eMysql, "ER_DATABASE_NAME", INT2NUM(ER_DATABASE_NAME));
    rb_define_const(eMysql, "ER_TABLE_NAME", INT2NUM(ER_TABLE_NAME));
    rb_define_const(eMysql, "ER_PARTITION_NAME", INT2NUM(ER_PARTITION_NAME));
    rb_define_const(eMysql, "ER_SUBPARTITION_NAME", INT2NUM(ER_SUBPARTITION_NAME));
    rb_define_const(eMysql, "ER_TEMPORARY_NAME", INT2NUM(ER_TEMPORARY_NAME));
    rb_define_const(eMysql, "ER_RENAMED_NAME", INT2NUM(ER_RENAMED_NAME));
    rb_define_const(eMysql, "ER_TOO_MANY_CONCURRENT_TRXS", INT2NUM(ER_TOO_MANY_CONCURRENT_TRXS));
    rb_define_const(eMysql, "ER_DEBUG_SYNC_TIMEOUT", INT2NUM(ER_DEBUG_SYNC_TIMEOUT));
    rb_define_const(eMysql, "ER_DEBUG_SYNC_HIT_LIMIT", INT2NUM(ER_DEBUG_SYNC_HIT_LIMIT));
    rb_define_const(eMysql, "ER_DUP_SIGNAL_SET", INT2NUM(ER_DUP_SIGNAL_SET));
    rb_define_const(eMysql, "ER_SIGNAL_WARN", INT2NUM(ER_SIGNAL_WARN));
    rb_define_const(eMysql, "ER_SIGNAL_NOT_FOUND", INT2NUM(ER_SIGNAL_NOT_FOUND));
    rb_define_const(eMysql, "ER_SIGNAL_EXCEPTION", INT2NUM(ER_SIGNAL_EXCEPTION));
    rb_define_const(eMysql, "ER_RESIGNAL_WITHOUT_ACTIVE_HANDLER", INT2NUM(ER_RESIGNAL_WITHOUT_ACTIVE_HANDLER));
    rb_define_const(eMysql, "ER_SIGNAL_BAD_CONDITION_TYPE", INT2NUM(ER_SIGNAL_BAD_CONDITION_TYPE));
    rb_define_const(eMysql, "ER_COND_ITEM_TOO_LONG", INT2NUM(ER_COND_ITEM_TOO_LONG));
    rb_define_const(eMysql, "ER_UNKNOWN_LOCALE", INT2NUM(ER_UNKNOWN_LOCALE));
    rb_define_const(eMysql, "ER_SLAVE_IGNORE_SERVER_IDS", INT2NUM(ER_SLAVE_IGNORE_SERVER_IDS));
    rb_define_const(eMysql, "ER_QUERY_CACHE_DISABLED", INT2NUM(ER_QUERY_CACHE_DISABLED));
    rb_define_const(eMysql, "ER_SAME_NAME_PARTITION_FIELD", INT2NUM(ER_SAME_NAME_PARTITION_FIELD));
    rb_define_const(eMysql, "ER_PARTITION_COLUMN_LIST_ERROR", INT2NUM(ER_PARTITION_COLUMN_LIST_ERROR));
    rb_define_const(eMysql, "ER_WRONG_TYPE_COLUMN_VALUE_ERROR", INT2NUM(ER_WRONG_TYPE_COLUMN_VALUE_ERROR));
    rb_define_const(eMysql, "ER_TOO_MANY_PARTITION_FUNC_FIELDS_ERROR", INT2NUM(ER_TOO_MANY_PARTITION_FUNC_FIELDS_ERROR));
    rb_define_const(eMysql, "ER_MAXVALUE_IN_VALUES_IN", INT2NUM(ER_MAXVALUE_IN_VALUES_IN));
    rb_define_const(eMysql, "ER_TOO_MANY_VALUES_ERROR", INT2NUM(ER_TOO_MANY_VALUES_ERROR));
    rb_define_const(eMysql, "ER_ROW_SINGLE_PARTITION_FIELD_ERROR", INT2NUM(ER_ROW_SINGLE_PARTITION_FIELD_ERROR));
    rb_define_const(eMysql, "ER_FIELD_TYPE_NOT_ALLOWED_AS_PARTITION_FIELD", INT2NUM(ER_FIELD_TYPE_NOT_ALLOWED_AS_PARTITION_FIELD));
    rb_define_const(eMysql, "ER_PARTITION_FIELDS_TOO_LONG", INT2NUM(ER_PARTITION_FIELDS_TOO_LONG));
    rb_define_const(eMysql, "ER_BINLOG_ROW_ENGINE_AND_STMT_ENGINE", INT2NUM(ER_BINLOG_ROW_ENGINE_AND_STMT_ENGINE));
    rb_define_const(eMysql, "ER_BINLOG_ROW_MODE_AND_STMT_ENGINE", INT2NUM(ER_BINLOG_ROW_MODE_AND_STMT_ENGINE));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_AND_STMT_ENGINE", INT2NUM(ER_BINLOG_UNSAFE_AND_STMT_ENGINE));
    rb_define_const(eMysql, "ER_BINLOG_ROW_INJECTION_AND_STMT_ENGINE", INT2NUM(ER_BINLOG_ROW_INJECTION_AND_STMT_ENGINE));
    rb_define_const(eMysql, "ER_BINLOG_STMT_MODE_AND_ROW_ENGINE", INT2NUM(ER_BINLOG_STMT_MODE_AND_ROW_ENGINE));
    rb_define_const(eMysql, "ER_BINLOG_ROW_INJECTION_AND_STMT_MODE", INT2NUM(ER_BINLOG_ROW_INJECTION_AND_STMT_MODE));
    rb_define_const(eMysql, "ER_BINLOG_MULTIPLE_ENGINES_AND_SELF_LOGGING_ENGINE", INT2NUM(ER_BINLOG_MULTIPLE_ENGINES_AND_SELF_LOGGING_ENGINE));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_LIMIT", INT2NUM(ER_BINLOG_UNSAFE_LIMIT));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_INSERT_DELAYED", INT2NUM(ER_BINLOG_UNSAFE_INSERT_DELAYED));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_SYSTEM_TABLE", INT2NUM(ER_BINLOG_UNSAFE_SYSTEM_TABLE));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_AUTOINC_COLUMNS", INT2NUM(ER_BINLOG_UNSAFE_AUTOINC_COLUMNS));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_UDF", INT2NUM(ER_BINLOG_UNSAFE_UDF));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_SYSTEM_VARIABLE", INT2NUM(ER_BINLOG_UNSAFE_SYSTEM_VARIABLE));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_SYSTEM_FUNCTION", INT2NUM(ER_BINLOG_UNSAFE_SYSTEM_FUNCTION));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_NONTRANS_AFTER_TRANS", INT2NUM(ER_BINLOG_UNSAFE_NONTRANS_AFTER_TRANS));
    rb_define_const(eMysql, "ER_MESSAGE_AND_STATEMENT", INT2NUM(ER_MESSAGE_AND_STATEMENT));
    rb_define_const(eMysql, "ER_SLAVE_CONVERSION_FAILED", INT2NUM(ER_SLAVE_CONVERSION_FAILED));
    rb_define_const(eMysql, "ER_SLAVE_CANT_CREATE_CONVERSION", INT2NUM(ER_SLAVE_CANT_CREATE_CONVERSION));
    rb_define_const(eMysql, "ER_INSIDE_TRANSACTION_PREVENTS_SWITCH_BINLOG_FORMAT", INT2NUM(ER_INSIDE_TRANSACTION_PREVENTS_SWITCH_BINLOG_FORMAT));
    rb_define_const(eMysql, "ER_PATH_LENGTH", INT2NUM(ER_PATH_LENGTH));
    rb_define_const(eMysql, "ER_WARN_DEPRECATED_SYNTAX_NO_REPLACEMENT", INT2NUM(ER_WARN_DEPRECATED_SYNTAX_NO_REPLACEMENT));
    rb_define_const(eMysql, "ER_WRONG_NATIVE_TABLE_STRUCTURE", INT2NUM(ER_WRONG_NATIVE_TABLE_STRUCTURE));
    rb_define_const(eMysql, "ER_WRONG_PERFSCHEMA_USAGE", INT2NUM(ER_WRONG_PERFSCHEMA_USAGE));
    rb_define_const(eMysql, "ER_WARN_I_S_SKIPPED_TABLE", INT2NUM(ER_WARN_I_S_SKIPPED_TABLE));
    rb_define_const(eMysql, "ER_INSIDE_TRANSACTION_PREVENTS_SWITCH_BINLOG_DIRECT", INT2NUM(ER_INSIDE_TRANSACTION_PREVENTS_SWITCH_BINLOG_DIRECT));
    rb_define_const(eMysql, "ER_STORED_FUNCTION_PREVENTS_SWITCH_BINLOG_DIRECT", INT2NUM(ER_STORED_FUNCTION_PREVENTS_SWITCH_BINLOG_DIRECT));
    rb_define_const(eMysql, "ER_SPATIAL_MUST_HAVE_GEOM_COL", INT2NUM(ER_SPATIAL_MUST_HAVE_GEOM_COL));
    rb_define_const(eMysql, "ER_TOO_LONG_INDEX_COMMENT", INT2NUM(ER_TOO_LONG_INDEX_COMMENT));
    rb_define_const(eMysql, "ER_LOCK_ABORTED", INT2NUM(ER_LOCK_ABORTED));
    rb_define_const(eMysql, "ER_DATA_OUT_OF_RANGE", INT2NUM(ER_DATA_OUT_OF_RANGE));
    rb_define_const(eMysql, "ER_WRONG_SPVAR_TYPE_IN_LIMIT", INT2NUM(ER_WRONG_SPVAR_TYPE_IN_LIMIT));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_MULTIPLE_ENGINES_AND_SELF_LOGGING_ENGINE", INT2NUM(ER_BINLOG_UNSAFE_MULTIPLE_ENGINES_AND_SELF_LOGGING_ENGINE));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_MIXED_STATEMENT", INT2NUM(ER_BINLOG_UNSAFE_MIXED_STATEMENT));
    rb_define_const(eMysql, "ER_INSIDE_TRANSACTION_PREVENTS_SWITCH_SQL_LOG_BIN", INT2NUM(ER_INSIDE_TRANSACTION_PREVENTS_SWITCH_SQL_LOG_BIN));
    rb_define_const(eMysql, "ER_STORED_FUNCTION_PREVENTS_SWITCH_SQL_LOG_BIN", INT2NUM(ER_STORED_FUNCTION_PREVENTS_SWITCH_SQL_LOG_BIN));
    rb_define_const(eMysql, "ER_FAILED_READ_FROM_PAR_FILE", INT2NUM(ER_FAILED_READ_FROM_PAR_FILE));
    rb_define_const(eMysql, "ER_VALUES_IS_NOT_INT_TYPE_ERROR", INT2NUM(ER_VALUES_IS_NOT_INT_TYPE_ERROR));
    rb_define_const(eMysql, "ER_ACCESS_DENIED_NO_PASSWORD_ERROR", INT2NUM(ER_ACCESS_DENIED_NO_PASSWORD_ERROR));
    rb_define_const(eMysql, "ER_SET_PASSWORD_AUTH_PLUGIN", INT2NUM(ER_SET_PASSWORD_AUTH_PLUGIN));
    rb_define_const(eMysql, "ER_GRANT_PLUGIN_USER_EXISTS", INT2NUM(ER_GRANT_PLUGIN_USER_EXISTS));
    rb_define_const(eMysql, "ER_TRUNCATE_ILLEGAL_FK", INT2NUM(ER_TRUNCATE_ILLEGAL_FK));
    rb_define_const(eMysql, "ER_PLUGIN_IS_PERMANENT", INT2NUM(ER_PLUGIN_IS_PERMANENT));
    rb_define_const(eMysql, "ER_SLAVE_HEARTBEAT_VALUE_OUT_OF_RANGE_MIN", INT2NUM(ER_SLAVE_HEARTBEAT_VALUE_OUT_OF_RANGE_MIN));
    rb_define_const(eMysql, "ER_SLAVE_HEARTBEAT_VALUE_OUT_OF_RANGE_MAX", INT2NUM(ER_SLAVE_HEARTBEAT_VALUE_OUT_OF_RANGE_MAX));
    rb_define_const(eMysql, "ER_STMT_CACHE_FULL", INT2NUM(ER_STMT_CACHE_FULL));
    rb_define_const(eMysql, "ER_MULTI_UPDATE_KEY_CONFLICT", INT2NUM(ER_MULTI_UPDATE_KEY_CONFLICT));
    rb_define_const(eMysql, "ER_TABLE_NEEDS_REBUILD", INT2NUM(ER_TABLE_NEEDS_REBUILD));
    rb_define_const(eMysql, "ER_INDEX_COLUMN_TOO_LONG", INT2NUM(ER_INDEX_COLUMN_TOO_LONG));
    rb_define_const(eMysql, "ER_ERROR_IN_TRIGGER_BODY", INT2NUM(ER_ERROR_IN_TRIGGER_BODY));
    rb_define_const(eMysql, "ER_ERROR_IN_UNKNOWN_TRIGGER_BODY", INT2NUM(ER_ERROR_IN_UNKNOWN_TRIGGER_BODY));
    rb_define_const(eMysql, "ER_INDEX_CORRUPT", INT2NUM(ER_INDEX_CORRUPT));
    rb_define_const(eMysql, "ER_UNDO_RECORD_TOO_BIG", INT2NUM(ER_UNDO_RECORD_TOO_BIG));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_INSERT_IGNORE_SELECT", INT2NUM(ER_BINLOG_UNSAFE_INSERT_IGNORE_SELECT));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_INSERT_SELECT_UPDATE", INT2NUM(ER_BINLOG_UNSAFE_INSERT_SELECT_UPDATE));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_REPLACE_SELECT", INT2NUM(ER_BINLOG_UNSAFE_REPLACE_SELECT));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_CREATE_IGNORE_SELECT", INT2NUM(ER_BINLOG_UNSAFE_CREATE_IGNORE_SELECT));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_CREATE_REPLACE_SELECT", INT2NUM(ER_BINLOG_UNSAFE_CREATE_REPLACE_SELECT));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_UPDATE_IGNORE", INT2NUM(ER_BINLOG_UNSAFE_UPDATE_IGNORE));
    rb_define_const(eMysql, "ER_PLUGIN_NO_UNINSTALL", INT2NUM(ER_PLUGIN_NO_UNINSTALL));
    rb_define_const(eMysql, "ER_PLUGIN_NO_INSTALL", INT2NUM(ER_PLUGIN_NO_INSTALL));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_WRITE_AUTOINC_SELECT", INT2NUM(ER_BINLOG_UNSAFE_WRITE_AUTOINC_SELECT));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_CREATE_SELECT_AUTOINC", INT2NUM(ER_BINLOG_UNSAFE_CREATE_SELECT_AUTOINC));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_INSERT_TWO_KEYS", INT2NUM(ER_BINLOG_UNSAFE_INSERT_TWO_KEYS));
    rb_define_const(eMysql, "ER_TABLE_IN_FK_CHECK", INT2NUM(ER_TABLE_IN_FK_CHECK));
    rb_define_const(eMysql, "ER_UNSUPPORTED_ENGINE", INT2NUM(ER_UNSUPPORTED_ENGINE));
    rb_define_const(eMysql, "ER_BINLOG_UNSAFE_AUTOINC_NOT_FIRST", INT2NUM(ER_BINLOG_UNSAFE_AUTOINC_NOT_FIRST));
    rb_define_const(eMysql, "ER_ERROR_LAST", INT2NUM(ER_ERROR_LAST));
}

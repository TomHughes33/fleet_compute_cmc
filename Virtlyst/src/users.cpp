/*
 * Copyright (C) 2019 Daniel Nicoletti <dantti12@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "users.h"
#include "virtlyst.h"
#include <iostream>

#include <QLoggingCategory>

#include <Cutelyst/Plugins/Authentication/authentication.h>
#include <Cutelyst/Plugins/Authentication/credentialpassword.h>
#include <Cutelyst/Plugins/Utils/Sql>

#include <QSqlQuery>
#include <QSqlError>

using namespace Cutelyst;

Users::Users(QObject *parent)
    : Controller(parent)
{
}

void Users::index(Context *c)
{
    /*QSqlQuery query = CPreparedSqlQueryThreadForDB(
                QStringLiteral("SELECT id, username "
                               "FROM users "
                               "ORDER BY username"),
                QStringLiteral("virtlyst"));*/
    try {
    pqxx::connection *psqlDB = Virtlyst::get_psqlDB();
    qDebug() << "In Users::index Querying users in Database " << psqlDB->dbname();
    const char *query = "SELECT id, username FROM users ORDER BY username";
    nontransaction notxn(*psqlDB);
    result res(notxn.exec(query));

    //if (query.exec()) {
    if (!res.empty()) {
//ToDo        c->setStash(QStringLiteral("users"), Sql::queryToList(query));
        c->setStash(QStringLiteral("users"), resultToList(res));
    } /*else {
        qDebug() << "error users" << query.lastError().text();*/
    }
    catch(const std::exception &e)
    {
	std::cerr << e.what() << std::endl;
        c->response()->setStatus(Response::InternalServerError);
    }
}

void Users::create(Context *c)
{
    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->req()->bodyParameters();
        c->setStash(QStringLiteral("user"), params);

        const QString password = params[QStringLiteral("password")];
        if (password.size() < 10) {
            c->setStash(QStringLiteral("error_msg"), QStringLiteral("Password too short"));
            return;
        }
        const QString pass = CredentialPassword::createPassword(password);

        /*QSqlQuery query = CPreparedSqlQueryThreadForDB(
                    QStringLiteral("INSERT INTO users "
                                   "(username, password) "
                                   "VALUES "
                                   "(:username, :password) "),
                    QStringLiteral("virtlyst"));
        query.bindValue(QStringLiteral(":username"), params.value(QStringLiteral("username")));
        query.bindValue(QStringLiteral(":password"), pass);
        if (query.exec()) {*/
	try {
    pqxx::connection *psqlDB = Virtlyst::get_psqlDB();
    qDebug() << "In Users::create inserting users into Database " << psqlDB->dbname();
    work txn(*psqlDB);
    psqlDB->prepare("insusquery", "INSERT INTO users (username, password) VALUES ($1, $2)");
    result res = txn.prepared("insusquery")((params.value(QStringLiteral("username"))).toLatin1().constData())(pass.toLatin1().constData()).exec();
    txn.commit();

            c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index"))));
            return;
    /*    } else {
            qDebug() << "error create user" << query.lastError().text();*/
    }
    catch(const std::exception &e)
    {
	std::cerr << e.what() << std::endl;
            c->response()->setStatus(Response::InternalServerError);
        }
    }
    c->setStash(QStringLiteral("user"), ParamsMultiMap{
                    {QStringLiteral("active"), QStringLiteral("on")},
                });
}

void Users::edit(Context *c, const QString &id)
{
    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->req()->bodyParameters();
        /*QSqlQuery query = CPreparedSqlQueryThreadForDB(
                    QStringLiteral("UPDATE users "
                                   "SET "
                                   "username=:username "
                                   "WHERE id=:id"),
                    QStringLiteral("virtlyst"));
        query.bindValue(QStringLiteral(":username"), params.value(QStringLiteral("username")));
        query.bindValue(QStringLiteral(":id"), id);
        if (query.exec()) {*/
	try {
    pqxx::connection *psqlDB = Virtlyst::get_psqlDB();
    qDebug() << "In Users::edit updating users into Database " << psqlDB->dbname();
    work txn(*psqlDB);
    psqlDB->prepare("updedquery", "UPDATE users SET username = $1 WHERE id = $2"); 
    result res = txn.prepared("updedquery")((params.value(QStringLiteral("username"))).toLatin1().constData())(id.toLatin1().constData()).exec();
    txn.commit();
            c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index"))));
            return;
       /* } else {
            qDebug() << "error users" << query.lastError().text();*/
    }
    catch(const std::exception &e)
    {
	std::cerr << e.what() << std::endl;
            c->response()->setStatus(Response::InternalServerError);
        }
    } else {
    /*    QSqlQuery query = CPreparedSqlQueryThreadForDB(
                    QStringLiteral("SELECT username "
                                   "FROM users "
                                   "WHERE id=:id"),
                    QStringLiteral("virtlyst"));
        query.bindValue(QStringLiteral(":id"), id);
        if (query.exec()) {*/
	    try {
    pqxx::connection *psqlDB = Virtlyst::get_psqlDB();
    qDebug() << "In Users::edit Querying users in Database " << psqlDB->dbname();
    nontransaction notxn(*psqlDB);
    psqlDB->prepare("seledquery","SELECT username FROM users WHERE id=$1");
    result res = notxn.prepared("seledquery")(id.toLatin1().constData()).exec();

    if(!res.empty()) {
//ToDo            c->setStash(QStringLiteral("user"), Sql::queryToHashObject(query));
            c->setStash(QStringLiteral("user"), resultToHashObject(res));
        } /*else {
            qDebug() << "error users" << query.lastError().text();*/
    }
    catch(const std::exception &e)
    {
	std::cerr << e.what() << std::endl;
            c->response()->setStatus(Response::InternalServerError);
        }
    }
    c->setStash(QStringLiteral("user_edit"), true);
    c->setStash(QStringLiteral("template"), QStringLiteral("users/create.html"));
}

void Users::change_password(Context *c, const QString &id)
{
    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->req()->bodyParameters();
        const QString password = params[QStringLiteral("password")];
        if (password != params[QStringLiteral("password_conf")]) {
            c->setStash(QStringLiteral("error_msg"), QStringLiteral("Password confirmation does not match"));
            return;
        }
        const QString pass = CredentialPassword::createPassword(password);

/*        QSqlQuery query = CPreparedSqlQueryThreadForDB(
                    QStringLiteral("UPDATE users "
                                   "SET password=:password "
                                   "WHERE id=:id"),
                    QStringLiteral("virtlyst"));
        query.bindValue(QStringLiteral(":password"), pass);
        query.bindValue(QStringLiteral(":id"), id);
        if (query.exec()) {*/
	try {
    pqxx::connection *psqlDB = Virtlyst::get_psqlDB();
    qDebug() << "In Users::change_password updating users into Database " << psqlDB->dbname();
    work txn(*psqlDB);
    psqlDB->prepare("updcpquery", "UPDATE users SET password = $1 WHERE id = $2"); 
    result res = txn.prepared("updcpquery")((params.value(QStringLiteral("username"))).toLatin1().constData())(id.toLatin1().constData()).exec();
    txn.commit();
            c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index"))));
            return;
        } /*else {
            qDebug() << "error users" << query.lastError().text();*/
    catch(const std::exception &e)
    {
	std::cerr << e.what() << std::endl;
            c->response()->setStatus(Response::InternalServerError);
        }
    }

    /*QSqlQuery query = CPreparedSqlQueryThreadForDB(
                QStringLiteral("SELECT username "
                               "FROM users "
                               "WHERE id=:id"),
                QStringLiteral("virtlyst"));
    query.bindValue(QStringLiteral(":id"), id);
    if (query.exec()) {*/
	    try {
    pqxx::connection *psqlDB = Virtlyst::get_psqlDB();
    qDebug() << "In Users::change_password Querying users in Database " << psqlDB->dbname();
    nontransaction notxn(*psqlDB);
    psqlDB->prepare("selcpquery","SELECT username FROM users WHERE id=$1");
    result res = notxn.prepared("selcpquery")(id.toLatin1().constData()).exec();

    if(!res.empty()) {
//ToDo        c->setStash(QStringLiteral("user"), Sql::queryToHashObject(query));
        c->setStash(QStringLiteral("user"), resultToHashObject(res));
    } /*else {
        qDebug() << "error users" << query.lastError().text();*/
    }
    catch(const std::exception &e)
    {
	std::cerr << e.what() << std::endl;
        c->response()->setStatus(Response::InternalServerError);
    }
}


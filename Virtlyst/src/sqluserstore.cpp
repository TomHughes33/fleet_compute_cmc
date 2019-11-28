/*
 * Copyright (C) 2018 Daniel Nicoletti <dantti12@gmail.com>
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
#include "sqluserstore.h"
#include "virtlyst.h"

#include <iostream>
#include <Cutelyst/Context>
#include <Cutelyst/Plugins/Utils/Sql>

#include <QJsonDocument>
#include <QJsonObject>

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>

#include <QLoggingCategory>

using namespace Cutelyst;

SqlUserStore::SqlUserStore(QObject *parent) : AuthenticationStore(parent)
{

}

Cutelyst::AuthenticationUser SqlUserStore::findUser(Cutelyst::Context *c, const Cutelyst::ParamsMultiMap &userinfo)
{
    Q_UNUSED(c)
/*    QSqlQuery query = CPreparedSqlQueryThreadForDB(
                QStringLiteral("SELECT * FROM users WHERE username = :username"),
                QStringLiteral("virtlyst"));
    query.bindValue(QStringLiteral(":username"), userinfo.value(QStringLiteral("username")));
    if (query.exec() && query.next()) {*/
    pqxx::connection *psqlDB = Virtlyst::get_psqlDB();
    qDebug() << "In SqlUserStore::findUser Querying users in Database " << psqlDB->dbname();
    nontransaction notxn(*psqlDB);
    psqlDB->prepare("seluserquery", "SELECT * FROM users WHERE username = $1");
    result res = notxn.prepared("seluserquery")((userinfo.value(QStringLiteral("username"))).toLatin1().constData()).exec();
    if(!res.empty()) {
        /* ToDo QVariant userId = query.value(QStringLiteral("id"));

        AuthenticationUser user(userId.toString());

        int columns = query.record().count();
        // send column headers
        QStringList cols;
        const QSqlRecord record = query.record();
        for (int j = 0; j < columns; ++j) {
            cols.append(record.fieldName(j));
        }

        for (int j = 0; j < columns; ++j) {
            user.insert(cols.at(j), query.value(j));
        }

        return user;*/
        QVariant userId = (res[0]["id"]).c_str();

        AuthenticationUser user(userId.toString());

    const tuple record = res[0];
    const int columns = record.size();
    QStringList cols;
    for (int i = 0; i < columns; ++i) {
        cols.append(record[i].name());
    }
        for (int i = 0; i < columns; ++i) {
            user.insert(cols.at(i), record[i].c_str());
	}
        return user;
    }

    return AuthenticationUser();
}

QString SqlUserStore::addUser(const ParamsMultiMap &user, bool replace)
{
    QSqlQuery query;
    try {
    pqxx::connection *psqlDB = Virtlyst::get_psqlDB();
    qDebug() << "In SqlUserStore::addUser inserting users in Database " << psqlDB->dbname();
    work txn(*psqlDB);
    if (replace) {
        /*query = CPreparedSqlQueryThreadForDB(
                    QStringLiteral("INSERT OR REPLACE INTO users "
                                   "(username, password) "
                                   "VALUES "
                                   "(:username, :password)"),
                    QStringLiteral("cmlyst"));*/
    psqlDB->prepare("insuserquery", "INSERT OR REPLACE INTO users (username, password) " \
		    		"VALUES ($1, $2)");
    } else {
       /* query = CPreparedSqlQueryThreadForDB(
                    QStringLiteral("INSERT INTO users "
                                   "(username, password) "
                                   "VALUES "
                                   "(:username, :password)"),
                    QStringLiteral("cmlyst"));*/
    psqlDB->prepare("insuserquery", "INSERT INTO users (username, password)  " \
		    		"VALUES ($1, $2)");
    }

    /*query.bindValue(QStringLiteral(":email"), user.value(QStringLiteral("username")));
    query.bindValue(QStringLiteral(":password"), user.value(QStringLiteral("password")));*/

    result res = txn.prepared("insuserquery")(user.value(QStringLiteral("username")).toLatin1().constData())(user.value(QStringLiteral("password")).toLatin1().constData()).exec();
    txn.commit();
    }
    catch(const std::exception &e)
    {
	std::cerr << "Failed to add new user:" << e.what() << std::endl;
        return QString();
    }
 /*ToDo   if (!query.exec()) {
        qDebug() << "Failed to add new user:" << query.lastError().databaseText() << user;
        return QString();
    }*/
    return user.value(QStringLiteral("username"));
}

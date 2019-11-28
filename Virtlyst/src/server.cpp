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
#include "server.h"

#include "virtlyst.h"


#include <Cutelyst/Plugins/StatusMessage>
#include <Cutelyst/Plugins/Utils/Sql>

#include <QSqlQuery>
#include <QSqlError>

#include <QUuid>
#include <QLoggingCategory>

Server::Server(Virtlyst *parent) : Controller(parent)
  , m_virtlyst(parent)
{

}

void Server::index(Context *c)
{
    c->setStash(QStringLiteral("template"), QStringLiteral("servers.html"));

    if (c->request()->isPost()) {
        const ParamsMultiMap params = c->request()->bodyParameters();
        if (params.contains(QStringLiteral("host_tcp_add"))) {
            const QString hostname = params[QStringLiteral("hostname")];
            const QString name = params[QStringLiteral("name")];
            const QString login = params[QStringLiteral("login")];
            const QString password = params[QStringLiteral("password")];

            createServer(ServerConn::ConnTCP, name, hostname, login, password);
        } else if (params.contains(QStringLiteral("host_ssh_add"))) {
            const QString hostname = params[QStringLiteral("hostname")];
            const QString name = params[QStringLiteral("name")];
            const QString login = params[QStringLiteral("login")];

            createServer(ServerConn::ConnSSH, name, hostname, login, QString());
        } else if (params.contains(QStringLiteral("host_tls_add"))) {
            const QString hostname = params[QStringLiteral("hostname")];
            const QString name = params[QStringLiteral("name")];
            const QString login = params[QStringLiteral("login")];
            const QString password = params[QStringLiteral("password")];

            createServer(ServerConn::ConnTLS, name, hostname, login, password);
        } else if (params.contains(QStringLiteral("host_socket_add"))) {
            const QString name = params[QStringLiteral("name")];

            createServer(ServerConn::ConnSocket, name, QStringLiteral("localhost"), QLatin1String(""), QString());
        } else if (params.contains(QStringLiteral("host_edit"))) {
            const int hostId = params[QStringLiteral("host_id")].toInt();
            const QString hostname = params[QStringLiteral("hostname")];
            const QString name = params[QStringLiteral("name")];
            const QString login = params[QStringLiteral("login")];
            const QString password = params[QStringLiteral("password")];

            updateServer(hostId, name, hostname, login, password);
        } else if (params.contains(QStringLiteral("host_del"))) {
            const int hostId = params[QStringLiteral("host_id")].toInt();

            deleteServer(hostId);
        }
        c->response()->redirect(c->uriFor(CActionFor(QStringLiteral("index"))));
        return;
    }

    c->setStash(QStringLiteral("servers"), QVariant::fromValue(m_virtlyst->servers(c)));
}

void Server::createServer(int type, const QString &name, const QString &hostname, const QString &login, const QString &password)
{
/*    QSqlQuery query = CPreparedSqlQueryThreadForDB(
                QStringLiteral("INSERT INTO servers_compute "
                               "(type, name, hostname, login, password) "
                               "VALUES "
                               "(:type, :name, :hostname, :login, :password)"),
                QStringLiteral("virtlyst"));
    query.bindValue(QStringLiteral(":type"), type);
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":hostname"), hostname);
    query.bindValue(QStringLiteral(":login"), login);
    query.bindValue(QStringLiteral(":password"), password);
    if (!query.exec()) {
        qWarning() << "Failed to add connection" << query.lastError().databaseText();
    }*/
    pqxx::connection *psqlDB = Virtlyst::get_psqlDB();
    qDebug() << "In Server::createServer inserting into Database " << psqlDB->dbname();
    work txn(*psqlDB);
    psqlDB->prepare("insscquery", "INSERT INTO servers_compute (type, name, hostname, login, password)" \
		    		"VALUES ($1, $2, $3, $4, $5)");
    result res = txn.prepared("insscquery")(type)(name.toLatin1().constData())(hostname.toLatin1().constData())(login.toLatin1().constData())(password.toLatin1().constData()).exec();
    txn.commit();
    
}

void Server::updateServer(int id, const QString &name, const QString &hostname, const QString &login, const QString &password)
{
    /*QSqlQuery query = CPreparedSqlQueryThreadForDB(
                QStringLiteral("UPDATE servers_compute "
                               "SET "
                               "name = :name, "
                               "hostname = :hostname, "
                               "login = :login, "
                               "password = :password "
                               "WHERE id = :id"),
                QStringLiteral("virtlyst"));
    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":hostname"), hostname);
    query.bindValue(QStringLiteral(":login"), login);
    query.bindValue(QStringLiteral(":password"), password);
    if (!query.exec()) {
        qWarning() << "Failed to update connection" << query.lastError().databaseText();
    }*/
    pqxx::connection *psqlDB = Virtlyst::get_psqlDB();
    qDebug() << "In Server::updateServer updating into Database " << psqlDB->dbname();
    work txn(*psqlDB);
    psqlDB->prepare("updscquery", "UPDATE servers_compute SET name = $1, hostname = $2, login = $3, password = $4 WHERE id = $5"); 
    result res = txn.prepared("updscquery")(name.toLatin1().constData())(hostname.toLatin1().constData())(login.toLatin1().constData())(password.toLatin1().constData())(id).exec();
    txn.commit();

}

void Server::deleteServer(int id)
{
    /*QSqlQuery query = CPreparedSqlQueryThreadForDB(
                QStringLiteral("DELETE FROM servers_compute WHERE id = :id"),
                QStringLiteral("virtlyst"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        qWarning() << "Failed to delete connection" << query.lastError().databaseText();
    }*/
    pqxx::connection *psqlDB = Virtlyst::get_psqlDB();
    qDebug() << "In Server::deleteServer deleting into Database " << psqlDB->dbname();
    work txn(*psqlDB);
    psqlDB->prepare("delscquery", "DELETE FROM servers_compute WHERE id = $1"); 
    result res = txn.prepared("delscquery")(id).exec();
    txn.commit();
}

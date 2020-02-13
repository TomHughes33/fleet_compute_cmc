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
#include "virtlyst.h"

#include <Cutelyst/Plugins/View/Grantlee/grantleeview.h>
#include <Cutelyst/Plugins/Utils/Sql>
#include <Cutelyst/Plugins/StatusMessage>
#include <Cutelyst/Plugins/Session/Session>
#include <Cutelyst/Plugins/Authentication/credentialpassword.h>
#include <Cutelyst/Plugins/Authentication/credentialhttp.h>
#include <Cutelyst/Plugins/Authentication/authenticationrealm.h>
#include <grantlee/engine.h>
#include <postgresql/libpq-fe.h>

#include <QFile>
#include <QMutexLocker>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <QUuid>
#include <QTranslator>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QCoreApplication>
#include <QProcess>

#include <libssh/libssh.h>
#include <stdlib.h>
#include <stdio.h>

#include "lib/connection.h"

#include "infrastructure.h"
#include "instances.h"
#include "info.h"
#include "overview.h"
#include "storages.h"
#include "networks.h"
#include "interfaces.h"
#include "secrets.h"
#include "server.h"
#include "console.h"
#include "create.h"
#include "users.h"
#include "root.h"
#include "ws.h"

#include "sqluserstore.h"

using namespace Cutelyst;

static QMutex mutex;

Q_LOGGING_CATEGORY(VIRTLYST, "virtlyst")

Virtlyst::Virtlyst(QObject *parent) : Application(parent)
{
    QCoreApplication::setApplicationName(QStringLiteral("Virtlyst"));
    QCoreApplication::setOrganizationName(QStringLiteral("Cutelyst"));
    QCoreApplication::setApplicationVersion(QStringLiteral("2.0.0"));
}

Virtlyst::~Virtlyst()
{
    qDeleteAll(m_connections);
}

bool Virtlyst::init()
{
    new Root(this);
    new Infrastructure(this);
    new Instances(this);
    new Info(this);
    new Overview(this);
    new Networks(this);
    new Interfaces(this);
    new Secrets(this);
    new Server(this);
    new Storages(this);
    new Console(this);
    new Create(this);
    new Users(this);
    new Ws(this);

    bool production = config(QStringLiteral("production")).toBool();
    qCDebug(VIRTLYST) << "Production" << production;

    m_dbPath = config(QStringLiteral("DatabasePath"),
                      QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QLatin1String("/virtlyst.sqlite")).toString();
    qCDebug(VIRTLYST) << "Database" << m_dbPath;
    if (!QFile::exists(m_dbPath)) {
       qDebug() << "Failed to open database" << m_dbPath;
    //    if (!createDB()) {
    //        qDebug() << "Failed to create database" << m_dbPath;
    //        return false;
    //    }
    //    QSqlDatabase::removeDatabase(QStringLiteral("db"));
    }

    auto templatePath = config(QStringLiteral("TemplatePath"), pathTo(QStringLiteral("root/src"))).toString();
    auto view = new GrantleeView(this);
    view->setCache(production);
    view->engine()->addDefaultLibrary(QStringLiteral("grantlee_i18ntags"));
    view->addTranslator(QLocale::system(), new QTranslator(this));
    view->setIncludePaths({ templatePath });

    auto store = new SqlUserStore;

    auto password = new CredentialPassword;
    password->setPasswordField(QStringLiteral("password"));
    password->setPasswordType(CredentialPassword::Hashed);

    auto realm = new AuthenticationRealm(store, password);

    auto basic = new CredentialHttp;
    basic->setType(CredentialHttp::Basic);
    basic->setPasswordType(CredentialHttp::Hashed);
    auto realm2 = new AuthenticationRealm(store, basic, "HTTP");

    new Session(this);

    auto auth = new Authentication(this);
    auth->addRealm(realm);
    auth->addRealm(realm2);

    new StatusMessage(this);

    return true;
}

bool Virtlyst::postFork()
{
    QMutexLocker locker(&mutex);

    auto dbDriver = config(QStringLiteral("DatabaseDriver")).toString();
    auto db = QSqlDatabase::addDatabase(dbDriver);

    if (dbDriver == "QSQLITE") {
        db.setDatabaseName(m_dbPath);
    }
    else {

        auto db_ip = config(QStringLiteral("DatabaseIP")).toString();
        auto db_port = config(QStringLiteral("DatabasePort")).toInt();
        auto db_user = config(QStringLiteral("DatabaseUser")).toString();
        auto db_name = config(QStringLiteral("DatabaseName")).toString();
        auto db_pwd = config(QStringLiteral("DatabasePwd")).toString();

        qDebug() << "database endpoint from config.ini" << db_ip << ":" << db_port;

        db.setUserName(db_user.toStdString().c_str());
        db.setPassword(db_pwd.toStdString().c_str());
        db.setDatabaseName(db_name.toStdString().c_str());
        db.setPort(db_port);
        db.setHostName(db_ip.toStdString().c_str());
    }

    if (!db.open()) {
        qCWarning(VIRTLYST) << "Failed to open database" << db.lastError().databaseText();
        return false;
    }

//    auto server = new ServerConn;
//    server->conn = new Connection(QStringLiteral("qemu:///system"), this);

//    m_connections.insert(QStringLiteral("1"), server);

    qCDebug(VIRTLYST) << "Database ready" << db.connectionName();

    updateConnections();

    return true;
}

QVector<ServerConn *> Virtlyst::servers(QObject *parent)
{
    QVector<ServerConn *> ret;
    auto it = m_connections.constBegin();
    while (it != m_connections.constEnd()) {
        ServerConn *conn = it.value()->clone(parent);
        ret.append(conn);
        ++it;
    }
    return ret;
}

Connection *Virtlyst::connection(const QString &id, QObject *parent)
{
    QString host;
    int port;
    ServerConn *server = m_connections.value(id);
    if (server && server->conn && server->conn->isAlive()) {
        return server->conn->clone(parent);
    } else if (server) {
        if (server->conn) {
            delete server->conn;
        }
        const QString hostname = server->url.host();
        if (hostname.contains(':')) {
            QRegExp separator(":");
            QStringList list = hostname.split(separator);
            host = list.at(0);
            port = list.at(1).toInt();
        } else {
            host = hostname;
            port = 22;
        }
        if(server->type == ServerConn::ConnSSH) {
	    // if(checkSSHconnection(host, port)){
        //         server->conn = server->isonline()
        //           ? new Connection(server->url, server->name, server)
        //           : nullptr;
	    // }
        qDebug() << "Removed connection " << host << ":"<< port;
	} else {
            server->conn = server->isonline()
              ? new Connection(server->url, server->name, server)
              : nullptr;
	}
        if (server->conn && server->conn->isAlive()) {
            return server->conn->clone(parent);
        }
    }

    return nullptr;
}

QString Virtlyst::prettyKibiBytes(quint64 kibiBytes)
{
    QString ret;
    const char* suffixes[6];
    suffixes[0] = " KB";
    suffixes[1] = " MB";
    suffixes[2] = " GB";
    suffixes[3] = " TB";
    suffixes[4] = " PB";
    suffixes[5] = " EB";
    uint s = 0; // which suffix to use
    double count = kibiBytes;
    while (count >= 1024 && s < 6) {
        count /= 1024;
        s++;
    }
    ret = QString::number(count, 'g', 3) + QLatin1String(suffixes[s]);
    return ret;
}

QStringList Virtlyst::keymaps()
{
    // list taken from http://qemu.weilnetz.de/qemu-doc.html#sec_005finvocation
    static QStringList ret = {
        QStringLiteral("ar"), QStringLiteral("da"), QStringLiteral("de"),
        QStringLiteral("de-ch"), QStringLiteral("en-gb"), QStringLiteral("en-us"),
        QStringLiteral("es"), QStringLiteral("et"), QStringLiteral("fi"),
        QStringLiteral("fo"), QStringLiteral("fr"), QStringLiteral("fr-be"),
        QStringLiteral("fr-ca"), QStringLiteral("fr-ch"), QStringLiteral("hr"),
        QStringLiteral("hu"), QStringLiteral("is"), QStringLiteral("it"),
        QStringLiteral("ja"), QStringLiteral("lt"), QStringLiteral("lv"),
        QStringLiteral("mk"), QStringLiteral("nl"), QStringLiteral("nl-be"),
        QStringLiteral("no"), QStringLiteral("pl"), QStringLiteral("pt"),
        QStringLiteral("pt-br"), QStringLiteral("ru"), QStringLiteral("sl"),
        QStringLiteral("sv"), QStringLiteral("th"), QStringLiteral("tr")
    };
    return ret;
}

bool Virtlyst::createDbFlavor(QSqlQuery &query, const QString &label, int memory, int vcpu, int disk)
{
    query.bindValue(QStringLiteral(":label"), label);
    query.bindValue(QStringLiteral(":memory"), memory);
    query.bindValue(QStringLiteral(":vcpu"), vcpu);
    query.bindValue(QStringLiteral(":disk"), disk);
    return query.exec();
}

void Virtlyst::updateConnections()
{
    QSqlQuery query = CPreparedSqlQueryThreadForDB(
                QStringLiteral("SELECT id, name, hostname, login, password, type, customer_number FROM servers_compute"),
                QStringLiteral("virtlyst"));
    if (!query.exec()) {
        qCWarning(VIRTLYST) << "Failed to get connections list";
    }

    QStringList ids;
    while (query.next()) {
        const QString id = query.value(0).toString();
        const QString name = query.value(1).toString();
        const QString hostname = query.value(2).toString();
        const QString login = query.value(3).toString();
        const QString password = query.value(4).toString();
        int type = query.value(5).toInt();
        ids << id;
        const QString cnumber = query.value(6).toString();

        qDebug() << "id: " << id;
        qDebug() << "name: " << name;
        qDebug() << "hostname: " << hostname;
        qDebug() << "login: " << login;
        qDebug() << "password: " << password;
        qDebug() << "cnumber: " << cnumber;

        // ServerConn *server = m_connections.value(id);
        // if (server) {
        //     if (server->name == name &&
        //             server->hostname == hostname &&
        //             server->login == login &&
        //             server->password == password &&
        //             server->type == type &&
        //             server->cnumber == cnumber) {
        //         continue;
        //     } else {
        //         delete server->conn;
        //     }
        // } else {
            
        // }
        ServerConn *server;
        server = new ServerConn(this);
        server->id = id;
        server->name = name;
        server->hostname = hostname;
        server->login = login;
        server->password = password;
        server->type = type;
        server->cnumber = cnumber;

        QUrl url;
	    QString host;
	    QString sshcmd;
	    int port;
        // switch (type) {
        // case ServerConn::ConnSocket:
        //     url = QStringLiteral("qemu:///system");
        //     break;
        // case ServerConn::ConnSSH:
        //     //url = QStringLiteral("qemu+ssh:///system");
        //     url = QStringLiteral("qemu+ssh:///system?no_verify=1&keyfile=/root/.ssh/id_rsa_hosting");
        //     if (hostname.contains(':')) {
        //         QRegExp separator(":");
        //         QStringList list = hostname.split(separator);
        //         url.setHost(list.at(0));
        //         url.setPort(list.at(1).toInt());
		//         host = list.at(0);
		//         port = list.at(1).toInt();
        //     } else {
        //       url.setHost(hostname);
	    //   host = hostname;
        //     }
        //     url.setUserName(login);

	    // //Execute command to avoid known host issue for new corp IP
        //     //qDebug() << "Before known host cmd ";
        //     sshcmd = "ssh-keygen -f /root/.ssh/known_hosts -R [" + host + "]:50022";
        //     QProcess::execute (sshcmd);
        //     //qDebug() << "After known host cmd ";
        //     break;
        // case ServerConn::ConnTCP:
        //     url = QStringLiteral("qemu+tcp:///system");
        //     url.setHost(hostname);
        //     url.setUserName(login);
        //     url.setPassword(password);
        //     break;
        // case ServerConn::ConnTLS:
        //     url = QStringLiteral("qemu+tls:///system");
        //     url.setHost(hostname);
        //     url.setUserName(login);
        //     url.setPassword(password);
        //     break;
        // }
        url = QStringLiteral("qemu+ssh:///system?no_verify=1&keyfile=/root/.ssh/id_rsa_hosting");
        server->url = url;

    //     switch (type) {
    //     case ServerConn::ConnSocket:
    //       server->conn = new Connection(url, name, server);
    //       break;
    //     case ServerConn::ConnSSH:
	//   if(checkSSHconnection(host, port)){
            
	//   }
    //       break;
    //     case ServerConn::ConnTCP:
    //     case ServerConn::ConnTLS:
    //       server->conn = server->isonline() ?
    //         new Connection(url, name, server)
    //         : nullptr;
    //       break;
    //     }
        server->conn = server->isonline() ?
              new Connection(url, name, server)
              : nullptr;
        m_connections.insert(id, server);
    }

    auto it = m_connections.begin();
    while (it != m_connections.end()) {
        if (!ids.contains(it.key())) {
            it.value()->deleteLater();
            it = m_connections.erase(it);
        } else {
            ++it;
        }
    }
}


/* 
bool Virtlyst::createDB()
{
    auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("db"));
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        qCWarning(VIRTLYST) << "Failed to open database" << db.lastError().databaseText();
        return false;
    }

    QSqlQuery query(db);
    qCDebug(VIRTLYST) << "Creating database" << m_dbPath;

    bool ret = query.exec(QStringLiteral("PRAGMA journal_mode = WAL"));
    qCDebug(VIRTLYST) << "PRAGMA journal_mode = WAL" << ret << query.lastError().databaseText();

    if (!query.exec(QStringLiteral("CREATE TABLE users "
                                   "( id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT"
                                   ", username TEXT UNIQUE NOT NULL "
                                   ", password TEXT NOT NULL "
                                   ")"))) {
        qCCritical(VIRTLYST) << "Error creating database" << query.lastError().text();
        return false;
    }

    if (!query.prepare(QStringLiteral("INSERT INTO users "
                                      "(username, password) "
                                      "VALUES "
                                      "('admin', :password)"))) {
        qCCritical(VIRTLYST) << "Error creating database" << query.lastError().text();
        return false;
    }
    const QString password = QString::fromLatin1("pandora123");
    query.bindValue(QStringLiteral(":password"), QString::fromLatin1(
                        CredentialPassword::createPassword(password.toUtf8(), QCryptographicHash::Sha256, 10000, 16, 16)));
    if (!query.exec()) {
        qCritical() << "Error creating database" << query.lastError().text();
        return false;
    }
    qCCritical(VIRTLYST) << "Created user admin with password:" << password;

    if (!query.exec(QStringLiteral("CREATE TABLE servers_compute "
                                   "( id integer NOT NULL PRIMARY KEY"
                                   ", name varchar(20) NOT NULL"
                                   ", hostname varchar(20) NOT NULL"
                                   ", login varchar(20)"
                                   ", password varchar(14)"
                                   ", type integer NOT NULL)"))) {
        qCCritical(VIRTLYST) << "Error creating database" << query.lastError().text();
        return false;
    }

    if (!query.exec(QStringLiteral("INSERT INTO servers_compute "
                                      "(id,name,hostname,type) "
                                      "VALUES "
                                      "(1,'Fleet Compute','localhost',4)"))) {
        qCCritical(VIRTLYST) << "Error INSERT INTO servers_compute database" << query.lastError().text();
        return false;
    }


    if (!query.exec(QStringLiteral("CREATE TABLE create_flavor "
                                   "( id integer NOT NULL PRIMARY KEY"
                                   ", label varchar(12) NOT NULL"
                                   ", memory integer NOT NULL"
                                   ", vcpu integer NOT NULL"
                                   ", disk integer NOT NULL)"))) {
        qCCritical(VIRTLYST) << "Error creating database" << query.lastError().text();
        return false;
    }

    if (!query.prepare(QStringLiteral("INSERT INTO create_flavor "
                                      "(label, memory, vcpu, disk) "
                                      "VALUES "
                                      "(:label, :memory, :vcpu, :disk)"))) {
        qCCritical(VIRTLYST) << "Error creating database" << query.lastError().text();
        return false;
    }

    createDbFlavor(query, QStringLiteral("micro"), 512, 1, 20);
    createDbFlavor(query, QStringLiteral("mini"), 1024, 2, 30);
    createDbFlavor(query, QStringLiteral("small"), 2048, 2, 40);
    createDbFlavor(query, QStringLiteral("medium"), 4096, 2, 60);
    createDbFlavor(query, QStringLiteral("large"), 8192, 4, 80);
    createDbFlavor(query, QStringLiteral("xlarge"), 16348, 8, 160);

    return true;
}


*/

bool ServerConn::isonline()
{
    QSqlQuery query = CPreparedSqlQueryThreadForDB(
        QStringLiteral("SELECT name FROM servers_compute WHERE last_identity_update > CURRENT_TIMESTAMP - INTERVAL '7 min' AND name=:name;"),
        QStringLiteral("virtlyst"));

    query.bindValue(QStringLiteral(":name"), name);

    if (!query.exec()) {
        qWarning() << "Failed to get online status" << query.lastError().databaseText();
    }
    if( query.next() ){
        return true;
    }
    return false;
}


bool ServerConn::alive()
{
    if (conn) {
        return conn->isAlive();
    }
    return false;
}

ServerConn *ServerConn::clone(QObject *parent)
{
    auto ret = new ServerConn(parent);
    ret->id = id;
    ret->name = name;
    ret->hostname = hostname;
    ret->login = login;
    ret->password = password;
    ret->cnumber = cnumber;
    ret->type = type;
    ret->url = url;

    QString host;
    int port;
    if (hostname.contains(':')) {
        QRegExp separator(":");
        QStringList list = hostname.split(separator);
        host = list.at(0);
        port = list.at(1).toInt();
    } else {
        host = hostname;
        port = 22;
    }
    // if (ret->isonline() && conn && !conn->isAlive()) {
    //     delete conn;
    //     if(type == ServerConn::ConnSSH) {
    //         if(checkSSHconnection(host, port)){
    //             conn = new Connection(url, name, this);
    //         }
    //     } else {
    //         conn = new Connection(url, name, this);
    //     }
    // }

    ret->conn = ret->isonline() ? conn->clone(ret) : nullptr;

    return ret;
}

bool checkSSHconnection(QString &host, int port)
{
  ssh_session my_ssh_session;
  int rc;
  bool ret;

  my_ssh_session = ssh_new();
  if (my_ssh_session == NULL)
    exit(-1);
  ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, host.toStdString().c_str());
  ssh_options_set(my_ssh_session, SSH_OPTIONS_PORT, &port);
  rc = ssh_connect(my_ssh_session);
  if (rc != SSH_OK)
  {
        qWarning() << "Error connecting to host - " << ssh_get_error(my_ssh_session);
    	ret = false;
  }
  else {
       qDebug() << "ssh connection successful ";
       ret = true;
  }

  ssh_disconnect(my_ssh_session);
  ssh_free(my_ssh_session);
  return ret;
}

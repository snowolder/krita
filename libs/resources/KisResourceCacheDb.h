/*
 * Copyright (C) 2018 Boudewijn Rempt <boud@valdyas.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KISRESOURCECACHEDB_H
#define KISRESOURCECACHEDB_H

#include <QObject>

#include <kritaresources_export.h>

#include <KisResourceStorage.h>

/**
 * @brief The KisResourceCacheDb class encapsulates the database that
 * caches information about the resources available to the user.
 *
 * KisApplication creates and initializes the database. All other methods
 * are static and can be used from anywhere.
 */
class KRITARESOURCES_EXPORT KisResourceCacheDb
{
public:

    static const QString dbLocationKey; ///< configuration key for the location of the database
    static const QString resourceCacheDbFilename; ///< filename of the database
    static const QString databaseVersion; ///< current schema version
    static QStringList storageTypes; ///< kinds of places where resources can be stored
    static QStringList disabledBundles; ///< the list of compatibility bundles that need to inactive by default

    /**
     * @brief isValid
     * @return true if the database has been correctly created, false if the database cannot be used
     */
    static bool isValid();

    /**
     * @brief lastError returns the last SQL error.
     */
    static QString lastError();

    /**
     * @brief initializes the database and updates the scheme if necessary. Does not actually
     * fill the database with pointers to resources.
     *
     * @param location the location of the database
     * @return true if the database has been initialized correctly
     */
    static bool initialize(const QString &location);

    /// Delete all storages that are Unknown or Memory and all resources that are marked temporary or belong to Unknown or Memory storages
    static void deleteTemporaryResources();

private:

    friend class KisResourceLocator;
    friend class TestResourceLocator;
    friend class TestResourceCacheDb;
    friend class KisTagModel;
    friend class KisResourceLoaderRegistry;

    explicit KisResourceCacheDb(); // Deleted
    ~KisResourceCacheDb(); // Deleted
    KisResourceCacheDb operator=(const KisResourceCacheDb&); // Deleted

    /**
     * @brief registerResourceType registers this resource type in the database
     * @param resourceType the string that represents the type
     * @return true if the type was registered or had already been registered
     */
    static bool registerResourceType(const QString &resourceType);

    static int resourceIdForResource(const QString &resourceName, const QString &resourceType, const QString &storageLocation);
    static bool resourceNeedsUpdating(int resourceId, QDateTime timestamp);

    /**
     * @brief addResourceVersion addes a new version of the resource to the database.
     * The resource itself already should be updated with the updated filename and version.
     * @param resourceId unique identifier for the resource
     * @param timestamp
     * @param storage
     * @param resource
     * @return true if the database was successfully updated
     */
    static bool addResourceVersion(int resourceId, QDateTime timestamp, KisResourceStorageSP storage, KoResourceSP resource);
    static bool addResource(KisResourceStorageSP storage, QDateTime timestamp, KoResourceSP resource, const QString &resourceType);
    static bool addResources(KisResourceStorageSP storage, QString resourceType);

    /// Make this resource inactive; this does not remove the resource from disk or from the database
    static bool removeResource(int resourceId);

    static bool tagResource(KisResourceStorageSP storage, const QString resourceName, KisTagSP tag, const QString &resourceType);
    static bool hasTag(const QString &url, const QString &resourceType);
    static bool linkTagToStorage(const QString &url, const QString &resourceType, const QString &storageLocation);
    static bool addTag(const QString &resourceType, const QString storageLocation, const QString url, const QString name, const QString comment);
    static bool addTags(KisResourceStorageSP storage, QString resourceType);

    static bool addStorage(KisResourceStorageSP storage, bool preinstalled);
    static bool deleteStorage(KisResourceStorageSP storage);
    static bool synchronizeStorage(KisResourceStorageSP storage);

    /**
     * @brief metaDataForId
     * @param id
     * @param tableName
     * @return
     */
    static QMap<QString, QVariant> metaDataForId(int id, const QString &tableName);

    /**
     * @brief setMetaDataForId removes all metadata for the given id and table name,
     *  and inserts the metadata in the metadata table.
     * @param id
     * @param tableName
     * @return true if successful, false if not
     */
    static bool updateMetaDataForId(const QMap<QString, QVariant> map, int id, const QString &tableName);
    static bool addMetaDataForId(const QMap<QString, QVariant> map, int id, const QString &tableName);

    static bool s_valid;
    static QString s_lastError;
};

#endif // KISRESOURCECACHEDB_H
#pragma once
#include <Core/SchemaInferenceMode.h>
#include <Disks/ObjectStorages/IObjectStorage.h>
#include <Formats/FormatSettings.h>
#include <Interpreters/ActionsDAG.h>
#include <Interpreters/Context_fwd.h>
#include <Parsers/IAST_fwd.h>
#include <Processors/Formats/IInputFormat.h>
#include <Processors/ISimpleTransform.h>
#include <Storages/ColumnsDescription.h>
#include <Storages/IStorage.h>
#include <Storages/ObjectStorage/DataLakes/DataLakeStorageSettings.h>
#include <Storages/ObjectStorage/DataLakes/IDataLakeMetadata.h>
#include <Storages/ObjectStorage/IObjectIterator.h>
#include <Storages/prepareReadingFromFormat.h>
#include <Common/threadPoolCallbackRunner.h>

#include <memory>

namespace DB
{

class ReadBufferIterator;
class SchemaCache;
class NamedCollection;
struct StorageObjectStorageSettings;
using StorageObjectStorageSettingsPtr = std::shared_ptr<StorageObjectStorageSettings>;

namespace ErrorCodes
{
extern const int NOT_IMPLEMENTED;
}


/**
 * A general class containing implementation for external table engines
 * such as StorageS3, StorageAzure, StorageHDFS.
 * Works with an object of IObjectStorage class.
 */
class StorageObjectStorage : public IStorage
{
public:
    class Configuration;
    using ConfigurationPtr = std::shared_ptr<Configuration>;
    using ConfigurationObserverPtr = std::weak_ptr<Configuration>;
    using ObjectInfo = RelativePathWithMetadata;
    using ObjectInfoPtr = std::shared_ptr<ObjectInfo>;
    using ObjectInfos = std::vector<ObjectInfoPtr>;

    struct QuerySettings
    {
        /// Insert settings:
        bool truncate_on_insert;
        bool create_new_file_on_insert;

        /// Schema inference settings:
        bool schema_inference_use_cache;
        SchemaInferenceMode schema_inference_mode;

        /// List settings:
        bool skip_empty_files;
        size_t list_object_keys_size;
        bool throw_on_zero_files_match;
        bool ignore_non_existent_file;
    };

    StorageObjectStorage(
        ConfigurationPtr configuration_,
        ObjectStoragePtr object_storage_,
        ContextPtr context_,
        const StorageID & table_id_,
        const ColumnsDescription & columns_,
        const ConstraintsDescription & constraints_,
        const String & comment,
        std::optional<FormatSettings> format_settings_,
        LoadingStrictnessLevel mode,
        bool distributed_processing_ = false,
        ASTPtr partition_by_ = nullptr,
        bool is_table_function_ = false,
        bool lazy_init = false);

    String getName() const override;

    void read(
        QueryPlan & query_plan,
        const Names & column_names,
        const StorageSnapshotPtr & storage_snapshot,
        SelectQueryInfo & query_info,
        ContextPtr local_context,
        QueryProcessingStage::Enum processed_stage,
        size_t max_block_size,
        size_t num_streams) override;

    SinkToStoragePtr
    write(const ASTPtr & query, const StorageMetadataPtr & metadata_snapshot, ContextPtr context, bool async_insert) override;

    void truncate(
        const ASTPtr & query, const StorageMetadataPtr & metadata_snapshot, ContextPtr local_context, TableExclusiveLockHolder &) override;

    bool supportsPartitionBy() const override { return true; }

    bool supportsSubcolumns() const override { return true; }

    bool supportsDynamicSubcolumns() const override { return true; }

    bool supportsTrivialCountOptimization(const StorageSnapshotPtr &, ContextPtr) const override { return true; }

    bool supportsSubsetOfColumns(const ContextPtr & context) const;

    bool prefersLargeBlocks() const override;

    bool parallelizeOutputAfterReading(ContextPtr context) const override;

    static SchemaCache & getSchemaCache(const ContextPtr & context, const std::string & storage_type_name);

    static ColumnsDescription resolveSchemaFromData(
        const ObjectStoragePtr & object_storage,
        const ConfigurationPtr & configuration,
        const std::optional<FormatSettings> & format_settings,
        std::string & sample_path,
        const ContextPtr & context);

    static std::string resolveFormatFromData(
        const ObjectStoragePtr & object_storage,
        const ConfigurationPtr & configuration,
        const std::optional<FormatSettings> & format_settings,
        std::string & sample_path,
        const ContextPtr & context);

    static std::pair<ColumnsDescription, std::string> resolveSchemaAndFormatFromData(
        const ObjectStoragePtr & object_storage,
        const ConfigurationPtr & configuration,
        const std::optional<FormatSettings> & format_settings,
        std::string & sample_path,
        const ContextPtr & context);

    void addInferredEngineArgsToCreateQuery(ASTs & args, const ContextPtr & context) const override;

    bool updateExternalDynamicMetadataIfExists(ContextPtr query_context) override;
    IDataLakeMetadata * getExternalMetadata(ContextPtr query_context);

    std::optional<UInt64> totalRows(ContextPtr query_context) const override;
    std::optional<UInt64> totalBytes(ContextPtr query_context) const override;

protected:
    /// Get path sample for hive partitioning implementation.
    String getPathSample(ContextPtr context);

    /// Creates ReadBufferIterator for schema inference implementation.
    static std::unique_ptr<ReadBufferIterator> createReadBufferIterator(
        const ObjectStoragePtr & object_storage,
        const ConfigurationPtr & configuration,
        const std::optional<FormatSettings> & format_settings,
        ObjectInfos & read_keys,
        const ContextPtr & context);

    /// Storage configuration (S3, Azure, HDFS, Local, DataLake).
    /// Contains information about table engine configuration
    /// and underlying storage access.
    ConfigurationPtr configuration;
    /// `object_storage` to allow direct access to data storage.
    const ObjectStoragePtr object_storage;
    const std::optional<FormatSettings> format_settings;
    /// Partition by expression from CREATE query.
    const ASTPtr partition_by;
    /// Whether this engine is a part of according Cluster engine implementation.
    /// (One of the reading replicas, not the initiator).
    const bool distributed_processing;
    /// Whether we need to call `configuration->update()`
    /// (e.g. refresh configuration) on each read() method call.
    bool update_configuration_on_read_write = true;

    LoggerPtr log;
};

class StorageObjectStorage::Configuration
{
public:
    Configuration() = default;
    virtual ~Configuration() = default;

    using Path = std::string;
    using Paths = std::vector<Path>;

    /// Initialize configuration from either AST or NamedCollection.
    static void initialize(
        Configuration & configuration_to_initialize,
        ASTs & engine_args,
        ContextPtr local_context,
        bool with_table_structure);

    /// Storage type: s3, hdfs, azure, local.
    virtual ObjectStorageType getType() const = 0;
    virtual std::string getTypeName() const = 0;
    /// Engine name: S3, HDFS, Azure.
    virtual std::string getEngineName() const = 0;
    /// Sometimes object storages have something similar to chroot or namespace, for example
    /// buckets in S3. If object storage doesn't have any namepaces return empty string.
    virtual std::string getNamespaceType() const { return "namespace"; }

    virtual Path getFullPath() const { return ""; }
    virtual Path getPath() const = 0;
    virtual void setPath(const Path & path) = 0;

    virtual const Paths & getPaths() const = 0;
    virtual void setPaths(const Paths & paths) = 0;

    virtual String getDataSourceDescription() const = 0;
    virtual String getNamespace() const = 0;

    virtual StorageObjectStorage::QuerySettings getQuerySettings(const ContextPtr &) const = 0;

    /// Add/replace structure and format arguments in the AST arguments if they have 'auto' values.
    virtual void addStructureAndFormatToArgsIfNeeded(
        ASTs & args, const String & structure_, const String & format_, ContextPtr context, bool with_structure)
        = 0;

    bool withPartitionWildcard() const;
    bool withGlobs() const { return isPathWithGlobs() || isNamespaceWithGlobs(); }
    bool withGlobsIgnorePartitionWildcard() const;
    bool isPathWithGlobs() const;
    bool isNamespaceWithGlobs() const;
    virtual std::string getPathWithoutGlobs() const;

    virtual bool isArchive() const { return false; }
    bool isPathInArchiveWithGlobs() const;
    virtual std::string getPathInArchive() const;

    virtual void check(ContextPtr context) const;
    virtual void validateNamespace(const String & /* name */) const { }

    virtual ObjectStoragePtr createObjectStorage(ContextPtr context, bool is_readonly) = 0;
    virtual bool isStaticConfiguration() const { return true; }

    virtual bool isDataLakeConfiguration() const { return false; }

    virtual std::optional<size_t> totalRows(ContextPtr) { return {}; }
    virtual std::optional<size_t> totalBytes(ContextPtr) { return {}; }

    virtual bool hasExternalDynamicMetadata() { return false; }

    virtual IDataLakeMetadata * getExternalMetadata() { return nullptr; }

    virtual std::shared_ptr<NamesAndTypesList> getInitialSchemaByPath(ContextPtr, const String &) const { return {}; }

    virtual std::shared_ptr<const ActionsDAG> getSchemaTransformer(ContextPtr, const String &) const { return {}; }

    virtual void modifyFormatSettings(FormatSettings &) const {}

    virtual ReadFromFormatInfo prepareReadingFromFormat(
        ObjectStoragePtr object_storage,
        const Strings & requested_columns,
        const StorageSnapshotPtr & storage_snapshot,
        bool supports_subset_of_columns,
        ContextPtr local_context);

    virtual std::optional<ColumnsDescription> tryGetTableStructureFromMetadata() const;

    virtual bool supportsFileIterator() const { return false; }
    virtual bool supportsWrites() const { return true; }

    virtual ObjectIterator iterate(
        const ActionsDAG * /* filter_dag */,
        std::function<void(FileProgress)> /* callback */,
        size_t /* list_batch_size */,
        ContextPtr /*context*/)
    {
        throw Exception(ErrorCodes::NOT_IMPLEMENTED, "Method iterate() is not implemented for configuration type {}", getTypeName());
    }

    /// Returns true, if metadata is of the latest version, false if unknown.
    virtual bool update(
        ObjectStoragePtr object_storage,
        ContextPtr local_context,
        bool if_not_updated_before,
        bool check_consistent_with_previous_metadata);

    virtual bool hasPositionDeleteTransformer(const ObjectInfoPtr & /*object_info*/) const { return false; }

    virtual std::shared_ptr<ISimpleTransform> getPositionDeleteTransformer(
        const ObjectInfoPtr & /*object_info*/,
        const Block & /*header*/,
        const std::optional<FormatSettings> & /*format_settings*/,
        ContextPtr /*context_*/) const
    {
        throw Exception(
            ErrorCodes::NOT_IMPLEMENTED,
            "Method getPositionDeleteTransformer() is not implemented for configuration type {}",
            getTypeName());
    }

    virtual const DataLakeStorageSettings & getDataLakeSettings() const
    {
        throw Exception(
            ErrorCodes::NOT_IMPLEMENTED, "Method getDataLakeSettings() is not implemented for configuration type {}", getTypeName());
    }

    String format = "auto";
    String compression_method = "auto";
    String structure = "auto";

protected:
    virtual void fromNamedCollection(const NamedCollection & collection, ContextPtr context) = 0;
    virtual void fromAST(ASTs & args, ContextPtr context, bool with_structure) = 0;

    void assertInitialized() const;

    bool initialized = false;
};

}

/**
 * @file polycall_program.h
 * @brief Program-First Architecture for LibPolyCall
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * This header defines the Program-First architecture model for LibPolyCall,
 * which prioritizes program semantics over language specifics. This approach
 * focuses on data flow and operation semantics rather than syntactic details,
 * allowing for language-agnostic interfaces.
 */

 #ifndef POLYCALL_POLYCALL_POLYCALL_PROGRAM_H_H
 #define POLYCALL_POLYCALL_POLYCALL_PROGRAM_H_H
 
 #include <stddef.h>
 #include <stdbool.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Program operation types
  */
 typedef enum {
     POLYCALL_OPERATION_NOOP = 0,
     POLYCALL_OPERATION_READ,
     POLYCALL_OPERATION_WRITE,
     POLYCALL_OPERATION_COMPUTE,
     POLYCALL_OPERATION_TRANSFORM,
     POLYCALL_OPERATION_AGGREGATE,
     POLYCALL_OPERATION_FILTER,
     POLYCALL_OPERATION_MAP,
     POLYCALL_OPERATION_REDUCE,
     POLYCALL_OPERATION_JOIN,
     POLYCALL_OPERATION_SORT,
     POLYCALL_OPERATION_GROUP,
     POLYCALL_OPERATION_USER = 0x1000   /**< Start of user-defined operations */
 } polycall_operation_type_t;
 
 /**
  * @brief Program data types
  */
 typedef enum {
     POLYCALL_DATA_TYPE_NONE = 0,
     POLYCALL_DATA_TYPE_BOOLEAN,
     POLYCALL_DATA_TYPE_INTEGER,
     POLYCALL_DATA_TYPE_FLOAT,
     POLYCALL_DATA_TYPE_STRING,
     POLYCALL_DATA_TYPE_BINARY,
     POLYCALL_DATA_TYPE_ARRAY,
     POLYCALL_DATA_TYPE_OBJECT,
     POLYCALL_DATA_TYPE_STREAM,
     POLYCALL_DATA_TYPE_FUNCTION,
     POLYCALL_DATA_TYPE_USER = 0x1000   /**< Start of user-defined data types */
 } polycall_data_type_t;
 
 /**
  * @brief Program flags
  */
 typedef enum {
     POLYCALL_PROGRAM_FLAG_NONE = 0,
     POLYCALL_PROGRAM_FLAG_IMMUTABLE = (1 << 0),
     POLYCALL_PROGRAM_FLAG_STATELESS = (1 << 1),
     POLYCALL_PROGRAM_FLAG_DETERMINISTIC = (1 << 2),
     POLYCALL_PROGRAM_FLAG_PARALLEL = (1 << 3),
     POLYCALL_PROGRAM_FLAG_ASYNC = (1 << 4),
     POLYCALL_PROGRAM_FLAG_OPTIMIZED = (1 << 5)
 } polycall_program_flags_t;
 
 /**
  * @brief Program node structure (opaque)
  */
 typedef struct polycall_program_node polycall_program_node_t;
 
 /**
  * @brief Program graph structure (opaque)
  */
 typedef struct polycall_program_graph polycall_program_graph_t;
 
 /**
  * @brief Program value structure
  */
 typedef struct {
     polycall_data_type_t type;          /**< Value type */
     union {
         bool boolean_value;             /**< Boolean value */
         int64_t integer_value;          /**< Integer value */
         double float_value;             /**< Float value */
         struct {                        /**< String value */
             const char* data;
             size_t length;
         } string_value;
         struct {                        /**< Binary value */
             const void* data;
             size_t size;
         } binary_value;
         struct {                        /**< Array value */
             void* items;
             size_t count;
             polycall_data_type_t item_type;
         } array_value;
         void* object_value;             /**< Object value */
         void* stream_value;             /**< Stream value */
         void* function_value;           /**< Function value */
         void* user_value;               /**< User-defined value */
     } data;
 } polycall_value_t;
 
 /**
  * @brief Program operation structure
  */
 typedef struct {
     polycall_operation_type_t type;     /**< Operation type */
     const char* name;                   /**< Operation name */
     size_t input_count;                 /**< Number of inputs */
     size_t output_count;                /**< Number of outputs */
     polycall_program_flags_t flags;     /**< Operation flags */
     void* user_data;                    /**< User-defined data */
     void (*execute)(                    /**< Execution function */
         polycall_core_context_t* ctx,
         const polycall_value_t* inputs,
         size_t input_count,
         polycall_value_t* outputs,
         size_t output_count,
         void* user_data
     );
 } polycall_operation_t;
 
 /**
  * @brief Create a program graph
  *
  * @param ctx Core context
  * @param graph Pointer to receive graph
  * @param name Graph name
  * @param flags Graph flags
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_program_create_graph(
     polycall_core_context_t* ctx,
     polycall_program_graph_t** graph,
     const char* name,
     polycall_program_flags_t flags
 );
 
 /**
  * @brief Destroy a program graph
  *
  * @param ctx Core context
  * @param graph Graph to destroy
  */
 void polycall_program_destroy_graph(
     polycall_core_context_t* ctx,
     polycall_program_graph_t* graph
 );
 
 /**
  * @brief Create a program node
  *
  * @param ctx Core context
  * @param graph Graph to add node to
  * @param node Pointer to receive node
  * @param operation Operation for the node
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_program_create_node(
     polycall_core_context_t* ctx,
     polycall_program_graph_t* graph,
     polycall_program_node_t** node,
     const polycall_operation_t* operation
 );
 
 /**
  * @brief Destroy a program node
  *
  * @param ctx Core context
  * @param node Node to destroy
  */
 void polycall_program_destroy_node(
     polycall_core_context_t* ctx,
     polycall_program_node_t* node
 );
 
 /**
  * @brief Connect two nodes
  *
  * @param ctx Core context
  * @param source Source node
  * @param source_output Source output index
  * @param target Target node
  * @param target_input Target input index
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_program_connect_nodes(
     polycall_core_context_t* ctx,
     polycall_program_node_t* source,
     size_t source_output,
     polycall_program_node_t* target,
     size_t target_input
 );
 
 /**
  * @brief Disconnect two nodes
  *
  * @param ctx Core context
  * @param source Source node
  * @param source_output Source output index
  * @param target Target node
  * @param target_input Target input index
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_program_disconnect_nodes(
     polycall_core_context_t* ctx,
     polycall_program_node_t* source,
     size_t source_output,
     polycall_program_node_t* target,
     size_t target_input
 );
 
 /**
  * @brief Set node input value
  *
  * @param ctx Core context
  * @param node Node to set input for
  * @param input_index Input index
  * @param value Input value
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_program_set_input(
     polycall_core_context_t* ctx,
     polycall_program_node_t* node,
     size_t input_index,
     const polycall_value_t* value
 );
 
 /**
  * @brief Get node output value
  *
  * @param ctx Core context
  * @param node Node to get output from
  * @param output_index Output index
  * @param value Pointer to receive output value
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_program_get_output(
     polycall_core_context_t* ctx,
     polycall_program_node_t* node,
     size_t output_index,
     polycall_value_t* value
 );
 
 /**
  * @brief Execute a program node
  *
  * @param ctx Core context
  * @param node Node to execute
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_program_execute_node(
     polycall_core_context_t* ctx,
     polycall_program_node_t* node
 );
 
 /**
  * @brief Execute a program graph
  *
  * @param ctx Core context
  * @param graph Graph to execute
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_program_execute_graph(
     polycall_core_context_t* ctx,
     polycall_program_graph_t* graph
 );
 
 /**
  * @brief Optimize a program graph
  *
  * @param ctx Core context
  * @param graph Graph to optimize
  * @param optimization_level Optimization level (0-3)
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_program_optimize_graph(
     polycall_core_context_t* ctx,
     polycall_program_graph_t* graph,
     uint32_t optimization_level
 );
 
 /**
  * @brief Validate a program graph
  *
  * @param ctx Core context
  * @param graph Graph to validate
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_program_validate_graph(
     polycall_core_context_t* ctx,
     polycall_program_graph_t* graph
 );
 
 /**
  * @brief Serialize a program graph
  *
  * @param ctx Core context
  * @param graph Graph to serialize
  * @param data Pointer to receive serialized data
  * @param size Pointer to receive serialized data size
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_program_serialize_graph(
     polycall_core_context_t* ctx,
     polycall_program_graph_t* graph,
     void** data,
     size_t* size
 );
 
 /**
  * @brief Deserialize a program graph
  *
  * @param ctx Core context
  * @param graph Pointer to receive deserialized graph
  * @param data Serialized data
  * @param size Serialized data size
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_program_deserialize_graph(
     polycall_core_context_t* ctx,
     polycall_program_graph_t** graph,
     const void* data,
     size_t size
 );
 
 /**
  * @brief Create a value
  *
  * @param ctx Core context
  * @param value Pointer to receive value
  * @param type Value type
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_program_create_value(
     polycall_core_context_t* ctx,
     polycall_value_t* value,
     polycall_data_type_t type
 );
 
 /**
  * @brief Destroy a value
  *
  * @param ctx Core context
  * @param value Value to destroy
  */
 void polycall_program_destroy_value(
     polycall_core_context_t* ctx,
     polycall_value_t* value
 );
 
 /**
  * @brief Create a standard operation
  *
  * @param ctx Core context
  * @param operation Pointer to receive operation
  * @param type Operation type
  * @param name Operation name
  * @return Error code indicating success or failure
  */
 polycall_core_error_t polycall_program_create_operation(
     polycall_core_context_t* ctx,
     polycall_operation_t* operation,
     polycall_operation_type_t type,
     const char* name
 );
 
 /**
 * @brief Create a custom operation
 *
 * @param ctx Core context
 * @param operation Pointer to receive operation
 * @param name Operation name
 * @param input_count Number of inputs
 * @param output_count Number of outputs
 * @param execute Execution function
 * @param user_data User-defined data
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_program_create_custom_operation(
    polycall_core_context_t* ctx,
    polycall_operation_t* operation,
    const char* name,
    size_t input_count,
    size_t output_count,
    void (*execute)(
        polycall_core_context_t* ctx,
        const polycall_value_t* inputs,
        size_t input_count,
        polycall_value_t* outputs,
        size_t output_count,
        void* user_data
    ),
    void* user_data
);

/**
 * @brief Destroy an operation
 *
 * @param ctx Core context
 * @param operation Operation to destroy
 */
void polycall_program_destroy_operation(
    polycall_core_context_t* ctx,
    polycall_operation_t* operation
);

/**
 * @brief Clone a program graph
 *
 * @param ctx Core context
 * @param source Source graph
 * @param clone Pointer to receive cloned graph
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_program_clone_graph(
    polycall_core_context_t* ctx,
    const polycall_program_graph_t* source,
    polycall_program_graph_t** clone
);

/**
 * @brief Get node information
 *
 * @param ctx Core context
 * @param node Node to query
 * @param operation Pointer to receive operation information
 * @param input_count Pointer to receive input count
 * @param output_count Pointer to receive output count
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_program_get_node_info(
    polycall_core_context_t* ctx,
    const polycall_program_node_t* node,
    polycall_operation_t* operation,
    size_t* input_count,
    size_t* output_count
);

/**
 * @brief Get graph statistics
 *
 * @param ctx Core context
 * @param graph Graph to query
 * @param node_count Pointer to receive node count
 * @param edge_count Pointer to receive edge count
 * @param depth Pointer to receive maximum graph depth
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_program_get_graph_stats(
    polycall_core_context_t* ctx,
    const polycall_program_graph_t* graph,
    size_t* node_count,
    size_t* edge_count,
    size_t* depth
);

/**
 * @brief Register a custom data type
 *
 * @param ctx Core context
 * @param type_id Pointer to receive type ID
 * @param type_name Type name
 * @param serializer Serialization function
 * @param deserializer Deserialization function
 * @param user_data User-defined data
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_program_register_data_type(
    polycall_core_context_t* ctx,
    polycall_data_type_t* type_id,
    const char* type_name,
    size_t (*serializer)(
        polycall_core_context_t* ctx,
        const void* data,
        void** buffer,
        size_t* buffer_size,
        void* user_data
    ),
    void* (*deserializer)(
        polycall_core_context_t* ctx,
        const void* buffer,
        size_t buffer_size,
        void* user_data
    ),
    void* user_data
);

/**
 * @brief Create a composite operation from a subgraph
 *
 * @param ctx Core context
 * @param operation Pointer to receive operation
 * @param subgraph Subgraph to encapsulate
 * @param name Operation name
 * @return Error code indicating success or failure
 */
polycall_core_error_t polycall_program_create_composite_operation(
    polycall_core_context_t* ctx,
    polycall_operation_t* operation,
    const polycall_program_graph_t* subgraph,
    const char* name
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_POLYCALL_POLYCALL_PROGRAM_H_H */
# Tune Advanced Auto Mixed Precision

## Background

### Numeric Stability

Using FP16 or BF16 will impact the model accuracy and lead to a [Numeric Stability](https://www.tensorflow.org/guide/mixed_precision) issue.

Some operations are **numerically-safe** for Float16/BFloat16. This means the operation based on FP16/BF16 has no obviously accuracy loss compared to FP32.

Some operations are **numerically-dangerous** for FP16/BF16. This means the operation based on FP16/BF16 has obviously accuracy loss compared to FP32.

### Configuration List

In order to achieve faster performance with strong numeric stability, Advanced Auto Mixed Precision (AMP) maintains four lists: ALLOWLIST, DENYLIST, INFERLIST, and CLEARLIST that let users manually configure a balance of performance and accuracy with FP16/BF16 if the default configuration doesn't provide the expected performance.

Set the lists according to the numerically-safe or numerically-dangerous type of the operations. The lists include the Operation Types of Tensorflow, and support fused Operations.


| List Name | Description                                                  |
| --------- | ------------------------------------------------------------ |
| ALLOWLIST | Include a set of operations that are always considered numerically-safe and performance-critical for FP16/BF16. The operations in ALLOWLIST are always converted to FP16/BF16. |
| DENYLIST  | Include a set of operations that are considered numerically-dangerous for execution in Float16/BFloat16). Additionally, they will affect the downstream nodes, making them numerically-dangerous too.  For example, in graph: Exp -> Add, the Add is numerically-dangerous due to the Exp). |
| INFERLIST | Include a set of operations that are considered numerically-safe for FP16/BF16, but they will be numerically-dangerous if impacted by an upstream node that is in the DENYLIST. |
| CLEARLIST | Include a set of operations that have no numerically-significant effects for FP16/BF16, and can run in FP16/BF16. According to the downstream/upstream nodes’ numerically-safe property, they could be set to Float16/BFloat16 if desired. They are used to reduce conversion between FP16/BF16 and FP32 in graph to improve performance.|

### Example of Mix Precision by List

Here is an example to explain the principle.

|List|Node Index|
|-|-|
|ALLOWLIST|6, 9|
|DENYLIST|1, 11|
|INFERLIST|2, 4, 7, 10|
|CLEARLIST|3, 5, 8|

![amp_list.png](images/amp_list.png)

Steps:

I. Set every node’s property according to the configuration of list (including default and custom setting).

II. Add nodes whose type is in ALLOWLIST to allow set.

   Node 6, 9.

III. Add nodes to deny set.

- The nodes in DENYLIST are added to deny set: Node 1, 11.
- The nodes in INFERLIST whose upstream nodes are in deny set (ignore upstream nodes which are in CLEARLIST): Node 2, 4.
- The nodes in CLEARLIST whose upstream and downstream nodes are in deny set: Node 3.

IV. Add nodes to allow set.

- The nodes in INFERLIST whose upstream nodes are in allow set: Node 7, 10.
- The nodes in CLEARLIST whose upstream or downstream nodes are in allow set: Node 5, 8.

V. Change nodes data type and insert Cast nodes.
   Insert Cast nodes between deny set (FP32) and allow set (FP16/BF16), which convert data type between FP32 and FP16/BF16.

### Rule to Improve Performance by the Configuration List

- Adding more nodes to Allow will increase the performance.
- Reducing Cast nodes in a different allow set will increase the performance.

### Usage

You can set these lists manually to tune Advance AMP by using the Python API, or by setting environment variables to override the default settings. Values set using the Python API will override those set using environment variables.
Settings are prioritized in this order: Python API > Environment Variable > Default Setting.

#### Python API

Create object:

  ```
  import intel_extension_for_tensorflow as itex
  auto_mixed_precision_options = itex.AutoMixedPrecosionOptions()
  ```

| Python APIs  |                                   Definition                         |
| -----------------------|------------------------------------------------------------------------|
| `itex.AutoMixedPrecosionOptions` | Use both 16-bit and 32-bit floating-point types during training, which makes models run faster and use less memory. Now, GPU supports both FP16 and BF16, and CPU only support BF16. |


### Python API Attribute & Environment Variable

| Attribute          | Environment Variable Names                 | Description                                                  |
| ------------------ | ------------------------------------------ | ------------------------------------------------------------ |
| `data_type`        | ITEX_AUTO_MIXED_PRECISION_DATA_TYPE        | Low precision data type used in Advanced AMP<br/>Three **options**: `DEFAULT_DATA_TYPE`,`FLOAT16`, `BFLOAT16`.<br> `DEFAULT_DATA_TYPE` is BF16 in CPU and GPU. <br/>CPU only supports BF16, GPU supports both FP16 and BF16. |
| `unsafe_force_all` | ITEX_AUTO_MIXED_PRECISION_UNSAFE_FORCE_ALL | Convert all FP32 operations to FP16/BF16 operations. <br>Only support Float16 data type. |
| `allowlist_add`    | ITEX_AUTO_MIXED_PRECISION_ALLOWLIST_ADD    | String. The operation types list added to ALLOWLIST. Use "," to split multiple operation types. |
| `denylist_add`     | ITEX_AUTO_MIXED_PRECISION_DENYLIST_ADD     | String. The operation types list added to DENYLIST. Use "," to split multiple operation types. |
| `inferlist_add`    | ITEX_AUTO_MIXED_PRECISION_INFERLIST_ADD    | String. The operation types list added to INFERLIST. Use "," to split multiple operation types.  |
| `clearlist_add`    | ITEX_AUTO_MIXED_PRECISION_CLEARLIST_ADD    | String. The operation types list added to CLEARLIST. Use "," to split multiple operation types.  |
| `allowlist_remove` | ITEX_AUTO_MIXED_PRECISION_ALLOWLIST_REMOVE | String. The operation types list removed from ALLOWLIST. Use "," to split multiple operation types. |
| `denylist_remove`  | ITEX_AUTO_MIXED_PRECISION_DENYLIST_REMOVE  | String. The operation types list removed from DENYLIST. Use "," to split multiple operation types. |
| `inferlist_remove` | ITEX_AUTO_MIXED_PRECISION_INFERLIST_REMOVE | String. The operation types list removed from INFERLIST. Use "," to split multiple operation types.  |
| `clearlist_remove` | ITEX_AUTO_MIXED_PRECISION_CLEARLIST_REMOVE | String. The operation types list removed from CLEARLIST. Use "," to split multiple operation types.  |


**Notes**:

Before adding an operation type to a list, remove it from the original list.

For example: AvgPool is in INFERLIST by default. To add it to ALLOWLIST, remove it from INFERLIST and add to ALLOWLIST.

### Environment Variable Difference with Stock TensorFlow

Advanced AMP has many extra operations. For example, ITEX_AUTO_MIXED_PRECISION_DATA_TYPE lets users use different data types (FP16/BF16) to speed up the model.

The following table shows the corresponding relationship between Advanced AMP and TensorFlow AMP environment variable names.

| Advanced AMP Environment Variable Name     | TensorFlow AMP Environment Variable Name                        |
| ------------------------------------------ | ------------------------------------------------------------ |
| ITEX_AUTO_MIXED_PRECISION_DATA_TYPE        | N/A                                                          |
| ITEX_AUTO_MIXED_PRECISION_LOG_PATH         | TF_AUTO_MIXED_PRECISION_GRAPH_REWRITE_LOG_PATH               |
| ITEX_AUTO_MIXED_PRECISION_UNSAFE_FORCE_ALL | TF_AUTO_MIXED_PRECISION_GRAPH_REWRITE_LEVEL="UNSAFE_FORCE_ALL" |
| ITEX_AUTO_MIXED_PRECISION_ALLOWLIST_ADD    | TF_AUTO_MIXED_PRECISION_GRAPH_REWRITE_ALLOWLIST_ADD          |
| ITEX_AUTO_MIXED_PRECISION_DENYLIST_ADD     | TF_AUTO_MIXED_PRECISION_GRAPH_REWRITE_DENYLIST_ADD           |
| ITEX_AUTO_MIXED_PRECISION_INFERLIST_ADD    | TF_AUTO_MIXED_PRECISION_GRAPH_REWRITE_INFERLIST_ADD          |
| ITEX_AUTO_MIXED_PRECISION_CLEARLIST_ADD    | TF_AUTO_MIXED_PRECISION_GRAPH_REWRITE_CLEARLIST_ADD          |
| ITEX_AUTO_MIXED_PRECISION_ALLOWLIST_REMOVE | TF_AUTO_MIXED_PRECISION_GRAPH_REWRITE_ALLOWLIST_REMOVE       |
| ITEX_AUTO_MIXED_PRECISION_DENYLIST_REMOVE  | TF_AUTO_MIXED_PRECISION_GRAPH_REWRITE_DENYLIST_REMOVE        |
| ITEX_AUTO_MIXED_PRECISION_INFERLIST_REMOVE | TF_AUTO_MIXED_PRECISION_GRAPH_REWRITE_INFERLIST_REMOVE       |
| ITEX_AUTO_MIXED_PRECISION_CLEARLIST_REMOVE | TF_AUTO_MIXED_PRECISION_GRAPH_REWRITE_CLEARLIST_REMOVE       |


## Usage

Steps:

I. Install Intel® Extension for TensorFlow* in running environment.

After Installing Intel® Extension for TensorFlow*, it will automatically activate as a plugin of stock Tensorflow.

Refer to [installation](/README.md#Install) instructions for more details.

II. Enable Advanced AMP.

With the default configuration, in most cases the Advanced AMP will balance accuracy with performance.

||Python API|Environment Variable|
|-|-|-|
|Basic (Default configuration)|`import intel_extension_for_tensorflow as itex`<br><br>`auto_mixed_precision_options = itex.AutoMixedPrecisionOptions()`<br>`auto_mixed_precision_options.data_type = itex.BFLOAT16 #itex.FLOAT16`<br><br>`graph_options = itex.GraphOptions()`<br>`graph_options.auto_mixed_precision_options=auto_mixed_precision_options`<br>`graph_options.auto_mixed_precision = itex.ON`<br><br>`config = itex.ConfigProto(graph_options=graph_options)`<br>`itex.set_config(config)`|`export ITEX_AUTO_MIXED_PRECISION=1`<br>`export ITEX_AUTO_MIXED_PRECISION_DATA_TYPE="BFLOAT16" #"FLOAT16"`<br>|


III. Use the Python API or environment variables to manually tune Advanced AMP for better performance, accuracy, or both.

||Python API|Environment Variable|
|-|-|-|
|Advanced Configuration|`auto_mixed_precision_options.allowlist_add= "AvgPool3D,AvgPool"`<br>`auto_mixed_precision_options.inferlist_remove = "AvgPool3D,AvgPool"`|`export ITEX_AUTO_MIXED_PRECISION_ALLOWLIST_ADD="AvgPool3D,AvgPool"`<br>`export ITEX_AUTO_MIXED_PRECISION_INFERLIST_REMOVE="AvgPool3D,AvgPool"`|

## Example

### End-to-end Example

Train a CNN model with Advanced AMP on GPU, and show the performance improvement. The following guide shows how to tune AMP manually. [Speed up Inference of Inception v4 by Advanced Automatic Mixed Precision on Intel CPU and GPU](./../../examples/infer_inception_v4_amp/README.md)

The first epoch may be slower because TensorFlow optimizes the model during the first run. In subsequent epochs, the run time will stabilize.

### Tuning Performance Example on MobileNet

Advanced AMP already provides more aggressive sub-graph fusion in more models. To achieve better performance, you may manually tune the Advanced AMP configuration list. It allows more operations to be converted to lower precision. However, usually it cannot provide more fusion chances.

Here is an example using MobileNet to tune manually.

I. Export the optimized graph by Advanced AMP with default configuration.  

Set environment variables:

```
export ITEX_AUTO_MIXED_PRECISION=1
export ITEX_AUTO_MIXED_PRECISION_LOG_PATH=/my/path/
```

After running the model inference by Intel® Extension for TensorFlow*, there will be 5 files in the path:
    
|   Log File    |   Explain |
|-------------- | --------- |
| .graphdef_AutoMixedPrecision_1657011814330.pb|post-optimization graph in binary format|
| .graphdef_AutoMixedPrecision_1657011814330.pb.txt|post-optimization graph in text format|
| .graphdef_preop_1657011815538.pb|pre-optimization graph in binary format|
| .graphdef_preop_1657011815538.pb.txt|pre-optimization graph in text format|
| .paintbuckets_AutoMixedPrecision_1657011814330.txt|include detailed info of ALLOWLIST, DENYLIST, INFERLIST, CLEARLIST|

II. Check the operation data type.

Use a tool (such as **Netron**) to open the graph file and check the operations' data type.

In MobileNet, only AvgPool and Softmax operations are **not** converted to BF16.

![itex-amp-example1.png](images/itex-amp-example1.png)


III. Convert operations to BFloat16 manually

Move AvgPool and Softmax to Allow List:

Alternative 1: set by environment variables:

```
export ITEX_AUTO_MIXED_PRECISION_INFERLIST_REMOVE=Softmax,AvgPool
export ITEX_AUTO_MIXED_PRECISION_ALLOWLIST_ADD=Softmax,AvgPool
```
Alternative 2: set by python API:

```
auto_mixed_precision_options.allowlist_add= "AvgPool3D,AvgPool"
auto_mixed_precision_options.inferlist_remove = "AvgPool3D,AvgPool"
```

IV. Execute for Advanced AMP with updated configuration

Run the model inference by Intel® Extension for TensorFlow* with above configuration tuning list, the performance will increase a little without a drop in accuracy, because only 2 operations are converted to BF16, occupying a lower rate over the whole runtime.
![itex-amp-example2.png](images/itex-amp-example2.png)

V. Continue tuning the Advanced AMP configuration list

Repeat the above steps to tune Advanced AMP, until you reach the peak performance with desired accuracy.

# Platform-specific build configurations.

load("@com_google_protobuf//:protobuf.bzl", "proto_gen")
load("//itex:workspace.bzl", "clean_dep")
load(
    "@rules_cc//cc:defs.bzl",
    _cc_binary = "cc_binary",
    _cc_import = "cc_import",
    _cc_library = "cc_library",
    _cc_test = "cc_test",
)

cc_binary = _cc_binary
cc_import = _cc_import
cc_library = _cc_library
cc_test = _cc_test

def cc_proto(name, src, deps = []):
    native.genrule(
        name = "%s_cc" % name,
        outs = ["include/protos/%s.pb.cc" % name, "include/protos/%s.pb.h" % name],
        cmd = "$(location @com_google_protobuf//:protoc) -I$(GENDIR)/external/local_config_tf/include/protos --cpp_out=$(GENDIR)/external/local_config_tf/include/protos $<",
        srcs = ["include/protos/%s" % src],
        tools = ["@com_google_protobuf//:protoc"],
    )
    native.cc_library(
        name = "%s_proto" % name,
        srcs = ["include/protos/%s.pb.cc" % name],
        hdrs = ["include/protos/%s.pb.h" % name],
        deps = [
            "@com_google_protobuf//:protobuf_headers",
            "@com_google_protobuf//:protobuf",
        ] + deps,
        copts = ["-I$(GENDIR)/external/local_config_tf/include/protos"],
    )

def if_static(extra_deps = [], otherwise = []):
    return otherwise

def if_not_windows(extra_deps = [], otherwise = []):
    return extra_deps

def well_known_proto_libs():
    """Set of standard protobuf protos, like Any and Timestamp.

    This list should be provided by protobuf.bzl, but it's not.
    """
    return [
        "@com_google_protobuf//:any_proto",
        "@com_google_protobuf//:api_proto",
        "@com_google_protobuf//:compiler_plugin_proto",
        "@com_google_protobuf//:descriptor_proto",
        "@com_google_protobuf//:duration_proto",
        "@com_google_protobuf//:empty_proto",
        "@com_google_protobuf//:field_mask_proto",
        "@com_google_protobuf//:source_context_proto",
        "@com_google_protobuf//:struct_proto",
        "@com_google_protobuf//:timestamp_proto",
        "@com_google_protobuf//:type_proto",
        "@com_google_protobuf//:wrappers_proto",
    ]

# Appends a suffix to a list of deps.
def tf_deps(deps, suffix):
    tf_deps = []

    # If the package name is in shorthand form (ie: does not contain a ':'),
    # expand it to the full name.
    for dep in deps:
        tf_dep = dep

        if not ":" in dep:
            dep_pieces = dep.split("/")
            tf_dep += ":" + dep_pieces[len(dep_pieces) - 1]

        tf_deps += [tf_dep + suffix]

    return tf_deps

# Modified from @cython//:Tools/rules.bzl
def pyx_library(
        name,
        deps = [],
        py_deps = [],
        srcs = [],
        testonly = None,
        srcs_version = "PY2AND3",
        **kwargs):
    """Compiles a group of .pyx / .pxd / .py files.

    First runs Cython to create .cpp files for each input .pyx or .py + .pxd
    pair. Then builds a shared object for each, passing "deps" to each cc_binary
    rule (includes Python headers by default). Finally, creates a py_library rule
    with the shared objects and any pure Python "srcs", with py_deps as its
    dependencies; the shared objects can be imported like normal Python files.

    Args:
      name: Name for the rule.
      deps: C/C++ dependencies of the Cython (e.g. Numpy headers).
      py_deps: Pure Python dependencies of the final library.
      srcs: .py, .pyx, or .pxd files to either compile or pass through.
      **kwargs: Extra keyword arguments passed to the py_library.
    """

    # First filter out files that should be run compiled vs. passed through.
    py_srcs = []
    pyx_srcs = []
    pxd_srcs = []
    for src in srcs:
        if src.endswith(".pyx") or (src.endswith(".py") and
                                    src[:-3] + ".pxd" in srcs):
            pyx_srcs.append(src)
        elif src.endswith(".py"):
            py_srcs.append(src)
        else:
            pxd_srcs.append(src)
        if src.endswith("__init__.py"):
            pxd_srcs.append(src)

    # Invoke cython to produce the shared object libraries.
    for filename in pyx_srcs:
        native.genrule(
            name = filename + "_cython_translation",
            srcs = [filename],
            outs = [filename.split(".")[0] + ".cpp"],
            # Optionally use PYTHON_BIN_PATH on Linux platforms so that python 3
            # works. Windows has issues with cython_binary so skip PYTHON_BIN_PATH.
            cmd = "PYTHONHASHSEED=0 $(location @cython//:cython_binary) --cplus $(SRCS) --output-file $(OUTS)",
            testonly = testonly,
            tools = ["@cython//:cython_binary"] + pxd_srcs,
        )

    shared_objects = []
    for src in pyx_srcs:
        stem = src.split(".")[0]
        shared_object_name = stem + ".so"
        native.cc_binary(
            name = shared_object_name,
            srcs = [stem + ".cpp"],
            deps = deps,  #+ ["@org_tensorflow//third_party/python_runtime:headers"],
            linkshared = 1,
            testonly = testonly,
        )
        shared_objects.append(shared_object_name)

    # Now create a py_library with these shared objects as data.
    native.py_library(
        name = name,
        srcs = py_srcs,
        deps = py_deps,
        srcs_version = srcs_version,
        data = shared_objects,
        testonly = testonly,
        **kwargs
    )

def _proto_cc_hdrs(srcs, use_grpc_plugin = False):
    ret = [s[:-len(".proto")] + ".pb.h" for s in srcs]
    if use_grpc_plugin:
        ret += [s[:-len(".proto")] + ".grpc.pb.h" for s in srcs]
    return ret

def _proto_cc_srcs(srcs, use_grpc_plugin = False):
    ret = [s[:-len(".proto")] + ".pb.cc" for s in srcs]
    if use_grpc_plugin:
        ret += [s[:-len(".proto")] + ".grpc.pb.cc" for s in srcs]
    return ret

def _proto_py_outs(srcs, use_grpc_plugin = False):
    ret = [s[:-len(".proto")] + "_pb2.py" for s in srcs]
    if use_grpc_plugin:
        ret += [s[:-len(".proto")] + "_pb2_grpc.py" for s in srcs]
    return ret

# Re-defined protocol buffer rule to allow building "header only" protocol
# buffers, to avoid duplicate registrations. Also allows non-iterable cc_libs
# containing select() statements.
def cc_proto_library(
        name,
        srcs = [],
        deps = [],
        cc_libs = [],
        include = None,
        protoc = "@com_google_protobuf//:protoc",
        internal_bootstrap_hack = False,
        use_grpc_plugin = False,
        use_grpc_namespace = False,
        make_default_target_header_only = False,
        protolib_name = None,
        protolib_deps = [],
        **kargs):
    """Bazel rule to create a C++ protobuf library from proto source files.

    Args:
      name: the name of the cc_proto_library.
      srcs: the .proto files of the cc_proto_library.
      deps: a list of dependency labels; must be cc_proto_library.
      cc_libs: a list of other cc_library targets depended by the generated
          cc_library.
      include: a string indicating the include path of the .proto files.
      protoc: the label of the protocol compiler to generate the sources.
      internal_bootstrap_hack: a flag indicate the cc_proto_library is used only
          for bootstraping. When it is set to True, no files will be generated.
          The rule will simply be a provider for .proto files, so that other
          cc_proto_library can depend on it.
      use_grpc_plugin: a flag to indicate whether to call the grpc C++ plugin
          when processing the proto files.
      use_grpc_namespace: the namespace for the grpc services.
      make_default_target_header_only: Controls the naming of generated
          rules. If True, the `name` rule will be header-only, and an _impl rule
          will contain the implementation. Otherwise the header-only rule (name
          + "_headers_only") must be referred to explicitly.
      protolib_name: the name for the proto library generated by this rule.
      protolib_deps: The dependencies to proto libraries.
      **kargs: other keyword arguments that are passed to cc_library.
    """

    wkt_deps = ["@com_google_protobuf//:cc_wkt_protos"]
    all_protolib_deps = protolib_deps + wkt_deps

    includes = []
    if include != None:
        includes = [include]
    if protolib_name == None:
        protolib_name = name

    if internal_bootstrap_hack:
        # For pre-checked-in generated files, we add the internal_bootstrap_hack
        # which will skip the codegen action.
        proto_gen(
            name = protolib_name + "_genproto",
            srcs = srcs,
            includes = includes,
            protoc = protoc,
            visibility = ["//visibility:public"],
            deps = [s + "_genproto" for s in all_protolib_deps],
        )

        # An empty cc_library to make rule dependency consistent.
        native.cc_library(
            name = name,
            **kargs
        )
        return

    grpc_cpp_plugin = None
    plugin_options = []
    if use_grpc_plugin:
        grpc_cpp_plugin = "//external:grpc_cpp_plugin"
        if use_grpc_namespace:
            plugin_options = ["services_namespace=grpc"]

    gen_srcs = _proto_cc_srcs(srcs, use_grpc_plugin)
    gen_hdrs = _proto_cc_hdrs(srcs, use_grpc_plugin)
    outs = gen_srcs + gen_hdrs

    proto_gen(
        name = protolib_name + "_genproto",
        srcs = srcs,
        outs = outs,
        gen_cc = 1,
        includes = includes,
        plugin = grpc_cpp_plugin,
        plugin_language = "grpc",
        plugin_options = plugin_options,
        protoc = protoc,
        visibility = ["//visibility:public"],
        deps = [s + "_genproto" for s in all_protolib_deps],
    )

    if use_grpc_plugin:
        cc_libs += select({
            clean_dep("//tensorflow:linux_s390x"): ["//external:grpc_lib_unsecure"],
            "//conditions:default": ["//external:grpc_lib"],
        })

    impl_name = name + "_impl"
    header_only_name = name + "_headers_only"
    header_only_deps = tf_deps(protolib_deps, "_cc_headers_only")

    if make_default_target_header_only:
        native.alias(
            name = name,
            actual = header_only_name,
            visibility = kargs["visibility"],
        )
    else:
        native.alias(
            name = name,
            actual = impl_name,
            visibility = kargs["visibility"],
        )

    native.cc_library(
        name = impl_name,
        srcs = gen_srcs,
        hdrs = gen_hdrs,
        deps = cc_libs + deps,
        includes = includes,
        alwayslink = 1,
        **kargs
    )
    native.cc_library(
        name = header_only_name,
        deps = [
            "@com_google_protobuf//:protobuf_headers",
        ] + header_only_deps + if_static([impl_name]),
        hdrs = gen_hdrs,
        **kargs
    )

# Re-defined protocol buffer rule to bring in the change introduced in commit
# https://github.com/google/protobuf/commit/294b5758c373cbab4b72f35f4cb62dc1d8332b68
# which was not part of a stable protobuf release in 04/2018.
# TODO(jsimsa): Remove this once the protobuf dependency version is updated
# to include the above commit.
def py_proto_library(
        name,
        srcs = [],
        deps = [],
        py_libs = [],
        py_extra_srcs = [],
        include = None,
        default_runtime = "@com_google_protobuf//:protobuf_python",
        protoc = "@com_google_protobuf//:protoc",
        use_grpc_plugin = False,
        **kargs):
    """Bazel rule to create a Python protobuf library from proto source files

    NOTE: the rule is only an internal workaround to generate protos. The
    interface may change and the rule may be removed when bazel has introduced
    the native rule.

    Args:
      name: the name of the py_proto_library.
      srcs: the .proto files of the py_proto_library.
      deps: a list of dependency labels; must be py_proto_library.
      py_libs: a list of other py_library targets depended by the generated
          py_library.
      py_extra_srcs: extra source files that will be added to the output
          py_library. This attribute is used for internal bootstrapping.
      include: a string indicating the include path of the .proto files.
      default_runtime: the implicitly default runtime which will be depended on by
          the generated py_library target.
      protoc: the label of the protocol compiler to generate the sources.
      use_grpc_plugin: a flag to indicate whether to call the Python C++ plugin
          when processing the proto files.
      **kargs: other keyword arguments that are passed to py_library.
    """
    outs = _proto_py_outs(srcs, use_grpc_plugin)

    includes = []
    if include != None:
        includes = [include]

    grpc_python_plugin = None
    if use_grpc_plugin:
        grpc_python_plugin = "//external:grpc_python_plugin"

    # Note: Generated grpc code depends on Python grpc module. This dependency
    # is not explicitly listed in py_libs. Instead, host system is assumed to
    # have grpc installed.

    proto_gen(
        name = name + "_genproto",
        srcs = srcs,
        outs = outs,
        gen_py = 1,
        includes = includes,
        plugin = grpc_python_plugin,
        plugin_language = "grpc",
        protoc = protoc,
        visibility = ["//visibility:public"],
        deps = [s + "_genproto" for s in deps],
    )

    if default_runtime and not default_runtime in py_libs + deps:
        py_libs = py_libs + [default_runtime]

    native.py_library(
        name = name,
        srcs = outs + py_extra_srcs,
        deps = py_libs + deps,
        imports = includes,
        **kargs
    )

def tf_proto_library_cc(
        name,
        srcs = [],
        has_services = None,
        protodeps = [],
        visibility = None,
        testonly = 0,
        cc_libs = [],
        cc_stubby_versions = None,
        cc_grpc_version = None,
        use_grpc_namespace = False,
        j2objc_api_version = 1,
        cc_api_version = 2,
        js_codegen = "jspb",
        create_service = False,
        create_java_proto = False,
        make_default_target_header_only = False):
    js_codegen = js_codegen  # unused argument
    native.filegroup(
        name = name + "_proto_srcs",
        srcs = srcs + tf_deps(protodeps, "_proto_srcs"),
        testonly = testonly,
        visibility = visibility,
    )
    _ignore = (create_service, create_java_proto)

    use_grpc_plugin = None
    if cc_grpc_version:
        use_grpc_plugin = True

    protolib_deps = tf_deps(protodeps, "")
    cc_deps = tf_deps(protodeps, "_cc")
    cc_name = name + "_cc"
    if not srcs:
        # This is a collection of sub-libraries. Build header-only and impl
        # libraries containing all the sources.
        proto_gen(
            name = name + "_genproto",
            protoc = "@com_google_protobuf//:protoc",
            visibility = ["//visibility:public"],
            deps = [s + "_genproto" for s in protolib_deps],
        )

        native.alias(
            name = cc_name + "_genproto",
            actual = name + "_genproto",
            testonly = testonly,
            visibility = visibility,
        )

        native.alias(
            name = cc_name + "_headers_only",
            actual = cc_name,
            testonly = testonly,
            visibility = visibility,
        )

        native.cc_library(
            name = cc_name,
            deps = cc_deps + ["@com_google_protobuf//:protobuf_headers"] + if_static([name + "_cc_impl"]),
            testonly = testonly,
            visibility = visibility,
        )
        native.cc_library(
            name = cc_name + "_impl",
            deps = [s + "_impl" for s in cc_deps] + ["@com_google_protobuf//:cc_wkt_protos"],
        )

        return

    cc_proto_library(
        name = cc_name,
        protolib_name = name,
        testonly = testonly,
        srcs = srcs,
        cc_libs = cc_libs + if_static(
            ["@com_google_protobuf//:protobuf"],
            ["@com_google_protobuf//:protobuf_headers"],
        ),
        copts = if_not_windows([
            "-Wno-unknown-warning-option",
            "-Wno-unused-but-set-variable",
            "-Wno-sign-compare",
        ]),
        make_default_target_header_only = make_default_target_header_only,
        protoc = "@com_google_protobuf//:protoc",
        use_grpc_plugin = use_grpc_plugin,
        use_grpc_namespace = use_grpc_namespace,
        visibility = visibility,
        deps = cc_deps + ["@com_google_protobuf//:cc_wkt_protos"],
        protolib_deps = protolib_deps,
    )

def tf_proto_library_py(
        name,
        srcs = [],
        protodeps = [],
        deps = [],
        visibility = None,
        testonly = 0,
        srcs_version = "PY2AND3",
        use_grpc_plugin = False):
    py_deps = tf_deps(protodeps, "_py")
    py_name = name + "_py"
    if not srcs:
        # This is a collection of sub-libraries. Build header-only and impl
        # libraries containing all the sources.
        proto_gen(
            name = py_name + "_genproto",
            protoc = "@com_google_protobuf//:protoc",
            visibility = ["//visibility:public"],
            deps = [s + "_genproto" for s in py_deps],
        )
        native.py_library(
            name = py_name,
            deps = py_deps + [clean_dep("@com_google_protobuf//:protobuf_python")],
            testonly = testonly,
            visibility = visibility,
        )
        return

    py_proto_library(
        name = py_name,
        testonly = testonly,
        srcs = srcs,
        default_runtime = clean_dep("@com_google_protobuf//:protobuf_python"),
        protoc = "@com_google_protobuf//:protoc",
        srcs_version = srcs_version,
        use_grpc_plugin = use_grpc_plugin,
        visibility = visibility,
        deps = deps + py_deps + [clean_dep("@com_google_protobuf//:protobuf_python")],
    )

def tf_jspb_proto_library(**kwargs):
    pass

def tf_proto_library(
        name,
        srcs = [],
        has_services = None,
        protodeps = [],
        visibility = None,
        testonly = 0,
        cc_libs = [],
        cc_api_version = 2,
        cc_grpc_version = None,
        use_grpc_namespace = False,
        j2objc_api_version = 1,
        js_codegen = "jspb",
        create_service = False,
        create_java_proto = False,
        make_default_target_header_only = False,
        exports = []):
    """Make a proto library, possibly depending on other proto libraries."""

    # TODO(b/145545130): Add docstring explaining what rules this creates and how
    # opensource projects importing TF in bazel can use them safely (i.e. w/o ODR or
    # ABI violations).
    _ignore = (js_codegen, exports, create_service, create_java_proto)

    native.proto_library(
        name = name,
        srcs = srcs,
        deps = protodeps + well_known_proto_libs(),
        visibility = visibility,
        testonly = testonly,
    )

    tf_proto_library_cc(
        name = name,
        testonly = testonly,
        srcs = srcs,
        cc_grpc_version = cc_grpc_version,
        use_grpc_namespace = use_grpc_namespace,
        cc_libs = cc_libs,
        make_default_target_header_only = make_default_target_header_only,
        protodeps = protodeps,
        visibility = visibility,
    )

    tf_proto_library_py(
        name = name,
        testonly = testonly,
        srcs = srcs,
        protodeps = protodeps,
        srcs_version = "PY2AND3",
        use_grpc_plugin = has_services,
        visibility = visibility,
    )

def tf_additional_env_hdrs():
    return []

def tf_additional_device_tracer_srcs():
    return ["device_tracer.cc"]

def tf_additional_test_deps():
    return []

def tf_kernel_tests_linkstatic():
    return 0

def tf_additional_lib_deps():
    """Additional dependencies needed to build TF libraries."""
    return [
        "@com_google_absl//absl/base:base",
        "@com_google_absl//absl/container:inlined_vector",
        "@com_google_absl//absl/types:span",
        "@com_google_absl//absl/types:optional",
    ] + if_static(
        [clean_dep("@nsync//:nsync_cpp")],
        [clean_dep("@nsync//:nsync_headers")],
    )

def tf_py_clif_cc(name, visibility = None, **kwargs):
    pass

def tf_pyclif_proto_library(
        name,
        proto_lib,
        proto_srcfile = "",
        visibility = None,
        **kwargs):
    native.filegroup(name = name)
    native.filegroup(name = name + "_pb2")

def tf_additional_rpc_deps():
    return []

def tf_additional_tensor_coding_deps():
    return []

def tf_fingerprint_deps():
    return [
        "@farmhash_archive//:farmhash",
    ]

def tf_protobuf_deps():
    return if_static(
        [
            clean_dep("@com_google_protobuf//:protobuf"),
        ],
        otherwise = [clean_dep("@com_google_protobuf//:protobuf_headers")],
    )

def tf_protobuf_compiler_deps():
    return if_static(
        [
            clean_dep("@com_google_protobuf//:protobuf"),
        ],
        otherwise = [clean_dep("@com_google_protobuf//:protobuf_headers")],
    )

def tf_portable_deps_no_runtime():
    return [
        "//third_party/eigen3",
        "@double_conversion//:double-conversion",
        "@nsync//:nsync_cpp",
        "@com_googlesource_code_re2//:re2",
        "@farmhash_archive//:farmhash",
    ]

def tf_google_mobile_srcs_no_runtime():
    return []

def tf_google_mobile_srcs_only_runtime():
    return []

def if_llvm_aarch64_available(then, otherwise = []):
    # TODO(b/...): The TF XLA build fails when adding a dependency on
    # @llvm/llvm-project/llvm:aarch64_target.
    return otherwise

def if_llvm_system_z_available(then, otherwise = []):
    return select({
        "//tensorflow:linux_s390x": then,
        "//conditions:default": otherwise,
    })

def tf_generate_proto_text_sources(name, srcs_relative_dir, srcs, protodeps = [], deps = [], visibility = None, compatible_with = None):
    out_hdrs = (
        [
            p.replace(".proto", ".pb_text.h")
            for p in srcs
        ] + [p.replace(".proto", ".pb_text-impl.h") for p in srcs]
    )
    out_srcs = [p.replace(".proto", ".pb_text.cc") for p in srcs]
    native.genrule(
        name = name + "_srcs",
        srcs = srcs + protodeps + [clean_dep("//itex/tools/proto_text:placeholder.txt")],
        outs = out_hdrs + out_srcs,
        visibility = visibility,
        cmd =
            "$(location //itex/tools/proto_text:gen_proto_text_functions) " +
            "$(@D) " + srcs_relative_dir + " $(SRCS)",
        tools = [
            clean_dep("//itex/tools/proto_text:gen_proto_text_functions"),
        ],
        compatible_with = compatible_with,
    )

    native.filegroup(
        name = name + "_hdrs",
        srcs = out_hdrs,
        visibility = visibility,
        compatible_with = compatible_with,
    )

    cc_library(
        compatible_with = compatible_with,
        name = name,
        srcs = out_srcs,
        hdrs = out_hdrs,
        visibility = visibility,
        deps = deps,
        alwayslink = 1,
    )

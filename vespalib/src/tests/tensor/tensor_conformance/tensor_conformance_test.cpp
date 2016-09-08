// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/vespalib/testkit/test_kit.h>
#include <vespa/vespalib/eval/test/tensor_conformance.h>
#include <vespa/vespalib/eval/simple_tensor_engine.h>
#include <vespa/vespalib/tensor/default_tensor_engine.h>

using vespalib::eval::SimpleTensorEngine;
using vespalib::eval::test::TensorConformance;
using vespalib::tensor::DefaultTensorEngine;

TEST_F("require that reference tensor implementation passes conformance test",
       TensorConformance(SimpleTensorEngine::ref()))
{
    TEST_DO(f1.run_all_tests());
}

IGNORE_TEST_F("require that production tensor implementation passes conformance test",
       TensorConformance(DefaultTensorEngine::ref()))
{
    TEST_DO(f1.run_all_tests());
}

TEST_MAIN() { TEST_RUN_ALL(); }

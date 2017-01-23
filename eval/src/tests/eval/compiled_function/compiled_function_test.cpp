// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/vespalib/testkit/test_kit.h>
#include <vespa/eval/eval/function.h>
#include <vespa/eval/eval/llvm/compiled_function.h>
#include <vespa/eval/eval/test/eval_spec.h>
#include <vespa/eval/eval/basic_nodes.h>
#include <vespa/vespalib/util/stringfmt.h>
#include <cmath>
#include <vespa/vespalib/test/insertion_operators.h>
#include <iostream>

using namespace vespalib::eval;

//-----------------------------------------------------------------------------

std::vector<vespalib::string> params_10({"p1", "p2", "p3", "p4", "p5", "p6", "p7", "p8", "p9", "p10"});

const char *expr_10 = "p1 + p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9 + p10";

TEST("require that separate parameter passing works") {
    CompiledFunction cf_10(Function::parse(params_10, expr_10), PassParams::SEPARATE);
    auto fun_10 = cf_10.get_function<10>();
    EXPECT_EQUAL(10.0, fun_10(1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0));
    EXPECT_EQUAL(50.0, fun_10(5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0));
    EXPECT_EQUAL(45.0, fun_10(0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0));
    EXPECT_EQUAL(45.0, fun_10(9.0, 8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0, 0.0));
}

TEST("require that array parameter passing works") {
    CompiledFunction arr_cf(Function::parse(params_10, expr_10), PassParams::ARRAY);
    auto arr_fun = arr_cf.get_function();
    EXPECT_EQUAL(10.0, arr_fun(&std::vector<double>({1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0})[0]));
    EXPECT_EQUAL(50.0, arr_fun(&std::vector<double>({5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0})[0]));
    EXPECT_EQUAL(45.0, arr_fun(&std::vector<double>({0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0})[0]));
    EXPECT_EQUAL(45.0, arr_fun(&std::vector<double>({9.0, 8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0, 0.0})[0]));
}

//-----------------------------------------------------------------------------

std::vector<vespalib::string> unsupported = {
    "sum(",
    "map(",
    "join(",
    "reduce(",
    "rename(",
    "tensor(",
    "concat("
};

bool is_unsupported(const vespalib::string &expression) {
    for (const auto &prefix: unsupported) {
        if (starts_with(expression, prefix)) {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------

struct MyEvalTest : test::EvalSpec::EvalTest {
    size_t pass_cnt = 0;
    size_t fail_cnt = 0;
    bool print_pass = false;
    bool print_fail = false;
    virtual void next_expression(const std::vector<vespalib::string> &param_names,
                                 const vespalib::string &expression) override
    {
        Function function = Function::parse(param_names, expression);
        ASSERT_TRUE(!function.has_error());
        bool is_supported = !is_unsupported(expression);
        bool has_issues = CompiledFunction::detect_issues(function);
        if (is_supported == has_issues) {
            const char *supported_str = is_supported ? "supported" : "not supported";
            const char *issues_str = has_issues ? "has issues" : "does not have issues";
            print_fail && fprintf(stderr, "expression %s is %s, but %s\n",
                                  expression.c_str(), supported_str, issues_str);
            ++fail_cnt;
        }
    }
    virtual void handle_case(const std::vector<vespalib::string> &param_names,
                             const std::vector<double> &param_values,
                             const vespalib::string &expression,
                             double expected_result) override
    {
        Function function = Function::parse(param_names, expression);
        ASSERT_TRUE(!function.has_error());
        bool is_supported = !is_unsupported(expression);
        bool has_issues = CompiledFunction::detect_issues(function);
        if (is_supported && !has_issues) {
            CompiledFunction cfun(function, PassParams::ARRAY);
            auto fun = cfun.get_function();
            ASSERT_EQUAL(cfun.num_params(), param_values.size());
            double result = fun(&param_values[0]);
            if (is_same(expected_result, result)) {
                print_pass && fprintf(stderr, "verifying: %s -> %g ... PASS\n",
                                      as_string(param_names, param_values, expression).c_str(),
                                      expected_result);
                ++pass_cnt;
            } else {
                print_fail && fprintf(stderr, "verifying: %s -> %g ... FAIL: got %g\n",
                                      as_string(param_names, param_values, expression).c_str(),
                                      expected_result, result);
                ++fail_cnt;
            }
        }
    }
};

TEST_FF("require that compiled evaluation passes all conformance tests", MyEvalTest(), test::EvalSpec()) {
    f1.print_fail = true;
    f2.add_all_cases();
    f2.each_case(f1);
    EXPECT_GREATER(f1.pass_cnt, 1000u);
    EXPECT_EQUAL(0u, f1.fail_cnt);
}

//-----------------------------------------------------------------------------

TEST("require that large (plugin) set membership checks work") {
    nodes::Array my_set;
    for(size_t i = 1; i <= 100; ++i) {
        my_set.add(nodes::Node_UP(new nodes::Number(i)));
    }
    nodes::DumpContext dump_ctx({});
    vespalib::string expr = vespalib::make_string("if(a in %s,1,0)",
                                                  my_set.dump(dump_ctx).c_str());
    // fprintf(stderr, "expression: %s\n", expr.c_str());
    CompiledFunction cf(Function::parse(expr), PassParams::SEPARATE);
    CompiledFunction arr_cf(Function::parse(expr), PassParams::ARRAY);
    auto fun = cf.get_function<1>();
    auto arr_fun = arr_cf.get_function();
    for (double value = 0.5; value <= 100.5; value += 0.5) {
        if (std::round(value) == value) {
            EXPECT_EQUAL(1.0, fun(value));
            EXPECT_EQUAL(1.0, arr_fun(&value));
        } else {
            EXPECT_EQUAL(0.0, fun(value));
            EXPECT_EQUAL(0.0, arr_fun(&value));            
        }
    }
}

//-----------------------------------------------------------------------------

CompiledFunction pass_fun(CompiledFunction cf) {
    auto fun = cf.get_function<2>();
    EXPECT_EQUAL(5.0, fun(2.0, 3.0));
    return cf;
}

TEST("require that compiled expression can be passed (moved) around") {
    CompiledFunction cf(Function::parse("a+b"), PassParams::SEPARATE);
    auto fun = cf.get_function<2>();
    EXPECT_EQUAL(4.0, fun(2.0, 2.0));
    CompiledFunction cf2 = pass_fun(std::move(cf));
    EXPECT_TRUE(cf.get_function<2>() == nullptr);
    auto fun2 = cf2.get_function<2>();
    EXPECT_TRUE(fun == fun2);
    EXPECT_EQUAL(10.0, fun(3.0, 7.0));
}

TEST("require that expressions with constant sub-expressions evaluate correctly") {
    CompiledFunction cf(Function::parse("if(1,2,10)+a+b+max(1,2)/1"), PassParams::SEPARATE);
    auto fun = cf.get_function<2>();
    EXPECT_EQUAL(7.0, fun(1.0, 2.0));
    EXPECT_EQUAL(11.0, fun(3.0, 4.0));
}

TEST("dump ir code to verify lazy casting") {
    CompiledFunction cf(Function::parse({"a", "b"}, "12==2+if(a==3&&a<10||b,10,5)"), PassParams::SEPARATE);
    auto fun = cf.get_function<2>();
    EXPECT_EQUAL(0.0, fun(0.0, 0.0));
    EXPECT_EQUAL(1.0, fun(0.0, 1.0));
    EXPECT_EQUAL(1.0, fun(3.0, 0.0));
    cf.dump();
}

TEST_MT("require that multithreaded compilation works", 64) {
    {
        CompiledFunction cf(Function::parse({"x", "y", "z", "w"}, "((x+1)*(y-1))/((z+1)/(w-1))"),
                            PassParams::SEPARATE);
        auto fun = cf.get_function<4>();
        EXPECT_EQUAL(1.0, fun(0.0, 2.0, 0.0, 2.0));
    }
    {
        CompiledFunction cf(Function::parse({"x", "y", "z", "w"}, "((x+1)*(y-1))/((z+1)/(w-1))"),
                            PassParams::SEPARATE);
        auto fun = cf.get_function<4>();
        EXPECT_EQUAL(4.0, fun(1.0, 3.0, 0.0, 2.0));
    }
    {
        CompiledFunction cf(Function::parse({"x", "y", "z", "w"}, "((x+1)*(y-1))/((z+1)/(w-1))"),
                            PassParams::SEPARATE);
        auto fun = cf.get_function<4>();
        EXPECT_EQUAL(2.0, fun(1.0, 3.0, 1.0, 2.0));
    }
    {
        CompiledFunction cf(Function::parse({"x", "y", "z", "w"}, "((x+1)*(y-1))/((z+1)/(w-1))"),
                            PassParams::SEPARATE);
        auto fun = cf.get_function<4>();
        EXPECT_EQUAL(8.0, fun(1.0, 3.0, 1.0, 5.0));
    }
}

//-----------------------------------------------------------------------------

TEST("require that function issues can be detected") {
    auto simple = Function::parse("a+b");
    auto complex = Function::parse("join(a,b,f(a,b)(a+b))");
    EXPECT_FALSE(simple.has_error());
    EXPECT_FALSE(complex.has_error());
    EXPECT_FALSE(CompiledFunction::detect_issues(simple));
    EXPECT_TRUE(CompiledFunction::detect_issues(complex));
    std::cerr << "Example function issues:" << std::endl
              << CompiledFunction::detect_issues(complex).list
              << std::endl;
}

//-----------------------------------------------------------------------------

TEST_MAIN() { TEST_RUN_ALL(); }
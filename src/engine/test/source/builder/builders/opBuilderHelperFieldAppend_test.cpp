#include <any>
#include <gtest/gtest.h>
#include <vector>

#include <baseTypes.hpp>

#include "opBuilderHelperMap.hpp"

using namespace base;

using builder::internals::builders::opBuilderHelperFieldAppend;
using json::Json;
using std::string;
using std::vector;

#define GTEST_COUT std::cout << "[          ] [ INFO ] "

const string helperFunctionName {"ef_append"};
const string sourceField {"fieldFrom"};
const string sourceFieldRef {string("$") + sourceField};
const string targetField {"/fieldTo"};

TEST(opBuilderHelperFieldAppend, Builds)
{
    auto tuple = std::make_tuple(targetField, helperFunctionName, vector<string> {sourceFieldRef});

    ASSERT_NO_THROW(opBuilderHelperFieldAppend(tuple));
}

TEST(opBuilderHelperFieldAppend, WrongSizeParameters)
{
    auto tuple = std::make_tuple(targetField, helperFunctionName, vector<string> {sourceFieldRef, "dummy_param"});

    ASSERT_THROW(opBuilderHelperFieldAppend(tuple), std::runtime_error);
}

TEST(opBuilderHelperFieldAppend, WrongTypeParameter)
{
    auto tuple = std::make_tuple(targetField, helperFunctionName, vector<string> {sourceField});

    ASSERT_THROW(opBuilderHelperFieldAppend(tuple), std::runtime_error);
}

TEST(opBuilderHelperFieldAppend, FailTargetNotFound)
{
    auto tuple = std::make_tuple(targetField, helperFunctionName, vector<string> {sourceFieldRef});

    const auto op = opBuilderHelperFieldAppend(tuple);
    auto event = std::make_shared<Json>(R"({
       "fieldFrom": {
          "key1": "value1",
          "key3": "value3"
       }
    })");

    const auto result = op->getPtr<Term<EngineOp>>()->getFn()(event);
    ASSERT_FALSE(result);
}

TEST(opBuilderHelperFieldAppend, FailReferenceNotFound)
{
    auto tuple = std::make_tuple(targetField, helperFunctionName, vector<string> {sourceFieldRef});

    const auto op = opBuilderHelperFieldAppend(tuple);
    auto event = std::make_shared<Json>(R"({
       "fieldTo": {
          "key1": "value1",
          "key3": "value3"
       }
    })");

    const auto result = op->getPtr<Term<EngineOp>>()->getFn()(event);
    ASSERT_FALSE(result);
}

TEST(opBuilderHelperFieldAppend, AppendToString)
{
    auto tuple = std::make_tuple(targetField, helperFunctionName, vector<string> {sourceFieldRef});
    const auto op = opBuilderHelperFieldAppend(tuple);

    auto event1 = std::make_shared<Json>(R"({
        "targetField": {
            "field": "value"
        },
        "sourceField": {
            "field": "new_value1"
        }
    })");
    Json expected1 {R"({
        "targetField": {
            "field": "new_value1"
        },
        "sourceField": {
            "field": "new_value1"
        }
    })"};

    auto event2 = std::make_shared<Json>(R"({
        "targetField": {
            "field": "value"
        },
        "sourceField": {
            "field": 123
        }
    })");
    Json expected2 {R"({
        "targetField": {
            "field": 123
        },
        "sourceField": {
            "field": 123
        }
    })"};

    auto event3 = std::make_shared<Json>(R"({
        "targetField": {
            "field": "value"
        },
        "sourceField": {
            "field": true
        }
    })");
    Json expected3 {R"({
        "targetField": {
            "field": true
        },
        "sourceField": {
            "field": true
        }
    })"};

    auto event4 = std::make_shared<Json>(R"({
        "targetField": {
            "field": "value"
        },
        "sourceField": {
            "field": null
        }
    })");
    Json expected4 {R"({
        "targetField": {
            "field": null
        },
        "sourceField": {
            "field": null
        }
    })"};

    auto event5 = std::make_shared<Json>(R"({
        "targetField": {
            "field": "value"
        },
        "sourceField": {
            "field": {
                "subfield": "subvalue"
            }
        }
    })");
    Json expected5 {R"({
        "targetField": {
            "field": {
                "subfield": "subvalue"
            }
        },
        "sourceField": {
            "field": {
                "subfield": "subvalue"
            }
        }
    })"};

    auto event6 = std::make_shared<Json>(R"({
        "targetField": {
            "field": "value"
        },
        "sourceField": {
            "field": ["123", 123, false, null]
        }
    })");
    Json expected6 {R"({
        "targetField": {
            "field": ["123", 123, false, null]
        },
        "sourceField": {
            "field": ["123", 123, false, null]
        }
    })"};

    const auto result1 = op->getPtr<Term<EngineOp>>()->getFn()(event1);
    ASSERT_TRUE(result1);
    ASSERT_EQ(expected1, *result1.payload());

    const auto result2 = op->getPtr<Term<EngineOp>>()->getFn()(event2);
    ASSERT_TRUE(result2);
    ASSERT_EQ(expected2, *result2.payload());

    const auto result3 = op->getPtr<Term<EngineOp>>()->getFn()(event3);
    ASSERT_TRUE(result3);
    ASSERT_EQ(expected3, *result3.payload());

    const auto result4 = op->getPtr<Term<EngineOp>>()->getFn()(event4);
    ASSERT_TRUE(result4);
    ASSERT_EQ(expected4, *result4.payload());

    const auto result5 = op->getPtr<Term<EngineOp>>()->getFn()(event5);
    ASSERT_TRUE(result5);
    ASSERT_EQ(expected5, *result5.payload());

    const auto result6 = op->getPtr<Term<EngineOp>>()->getFn()(event6);
    ASSERT_TRUE(result6);
    ASSERT_EQ(expected6, *result6.payload());
}

TEST(opBuilderHelperFieldAppend, AppendToInt)
{
    auto tuple = std::make_tuple(targetField, helperFunctionName, vector<string> {sourceFieldRef});
    const auto op = opBuilderHelperFieldAppend(tuple);

    auto event1 = std::make_shared<Json>(R"({
        "targetField": {
            "field": 404
        },
        "sourceField": {
            "field": "new_value1"
        }
    })");
    Json expected1 {R"({
        "targetField": {
            "field": "new_value1"
        },
        "sourceField": {
            "field": "new_value1"
        }
    })"};

    auto event2 = std::make_shared<Json>(R"({
        "targetField": {
            "field": 404
        },
        "sourceField": {
            "field": 123
        }
    })");
    Json expected2 {R"({
        "targetField": {
            "field": 123
        },
        "sourceField": {
            "field": 123
        }
    })"};

    auto event3 = std::make_shared<Json>(R"({
        "targetField": {
            "field": 404
        },
        "sourceField": {
            "field": true
        }
    })");
    Json expected3 {R"({
        "targetField": {
            "field": true
        },
        "sourceField": {
            "field": true
        }
    })"};

    auto event4 = std::make_shared<Json>(R"({
        "targetField": {
            "field": 404
        },
        "sourceField": {
            "field": null
        }
    })");
    Json expected4 {R"({
        "targetField": {
            "field": null
        },
        "sourceField": {
            "field": null
        }
    })"};

    auto event5 = std::make_shared<Json>(R"({
        "targetField": {
            "field": 404
        },
        "sourceField": {
            "field": {
                "subfield": "subvalue"
            }
        }
    })");
    Json expected5 {R"({
        "targetField": {
            "field": {
                "subfield": "subvalue"
            }
        },
        "sourceField": {
            "field": {
                "subfield": "subvalue"
            }
        }
    })"};

    auto event6 = std::make_shared<Json>(R"({
        "targetField": {
            "field": 404
        },
        "sourceField": {
            "field": ["123", 123, false, null]
        }
    })");
    Json expected6 {R"({
        "targetField": {
            "field": ["123", 123, false, null]
        },
        "sourceField": {
            "field": ["123", 123, false, null]
        }
    })"};

    const auto result1 = op->getPtr<Term<EngineOp>>()->getFn()(event1);
    ASSERT_TRUE(result1);
    ASSERT_EQ(expected1, *result1.payload());

    const auto result2 = op->getPtr<Term<EngineOp>>()->getFn()(event2);
    ASSERT_TRUE(result2);
    ASSERT_EQ(expected2, *result2.payload());

    const auto result3 = op->getPtr<Term<EngineOp>>()->getFn()(event3);
    ASSERT_TRUE(result3);
    ASSERT_EQ(expected3, *result3.payload());

    const auto result4 = op->getPtr<Term<EngineOp>>()->getFn()(event4);
    ASSERT_TRUE(result4);
    ASSERT_EQ(expected4, *result4.payload());

    const auto result5 = op->getPtr<Term<EngineOp>>()->getFn()(event5);
    ASSERT_TRUE(result5);
    ASSERT_EQ(expected5, *result5.payload());

    const auto result6 = op->getPtr<Term<EngineOp>>()->getFn()(event6);
    ASSERT_TRUE(result6);
    ASSERT_EQ(expected6, *result6.payload());
}

TEST(opBuilderHelperFieldAppend, AppendToFloat)
{
    auto tuple = std::make_tuple(targetField, helperFunctionName, vector<string> {sourceFieldRef});
    const auto op = opBuilderHelperFieldAppend(tuple);

    auto event1 = std::make_shared<Json>(R"({
        "targetField": {
            "field": 404.69
        },
        "sourceField": {
            "field": "new_value1"
        }
    })");
    Json expected1 {R"({
        "targetField": {
            "field": "new_value1"
        },
        "sourceField": {
            "field": "new_value1"
        }
    })"};

    auto event2 = std::make_shared<Json>(R"({
        "targetField": {
            "field": 404.69
        },
        "sourceField": {
            "field": 123
        }
    })");
    Json expected2 {R"({
        "targetField": {
            "field": 123
        },
        "sourceField": {
            "field": 123
        }
    })"};

    auto event3 = std::make_shared<Json>(R"({
        "targetField": {
            "field": 404.69
        },
        "sourceField": {
            "field": true
        }
    })");
    Json expected3 {R"({
        "targetField": {
            "field": true
        },
        "sourceField": {
            "field": true
        }
    })"};

    auto event4 = std::make_shared<Json>(R"({
        "targetField": {
            "field": 404.69
        },
        "sourceField": {
            "field": null
        }
    })");
    Json expected4 {R"({
        "targetField": {
            "field": null
        },
        "sourceField": {
            "field": null
        }
    })"};

    auto event5 = std::make_shared<Json>(R"({
        "targetField": {
            "field": 404.69
        },
        "sourceField": {
            "field": {
                "subfield": "subvalue"
            }
        }
    })");
    Json expected5 {R"({
        "targetField": {
            "field": {
                "subfield": "subvalue"
            }
        },
        "sourceField": {
            "field": {
                "subfield": "subvalue"
            }
        }
    })"};

    auto event6 = std::make_shared<Json>(R"({
        "targetField": {
            "field": 404.69
        },
        "sourceField": {
            "field": ["123", 123, false, null]
        }
    })");
    Json expected6 {R"({
        "targetField": {
            "field": ["123", 123, false, null]
        },
        "sourceField": {
            "field": ["123", 123, false, null]
        }
    })"};

    const auto result1 = op->getPtr<Term<EngineOp>>()->getFn()(event1);
    ASSERT_TRUE(result1);
    ASSERT_EQ(expected1, *result1.payload());

    const auto result2 = op->getPtr<Term<EngineOp>>()->getFn()(event2);
    ASSERT_TRUE(result2);
    ASSERT_EQ(expected2, *result2.payload());

    const auto result3 = op->getPtr<Term<EngineOp>>()->getFn()(event3);
    ASSERT_TRUE(result3);
    ASSERT_EQ(expected3, *result3.payload());

    const auto result4 = op->getPtr<Term<EngineOp>>()->getFn()(event4);
    ASSERT_TRUE(result4);
    ASSERT_EQ(expected4, *result4.payload());

    const auto result5 = op->getPtr<Term<EngineOp>>()->getFn()(event5);
    ASSERT_TRUE(result5);
    ASSERT_EQ(expected5, *result5.payload());

    const auto result6 = op->getPtr<Term<EngineOp>>()->getFn()(event6);
    ASSERT_TRUE(result6);
    ASSERT_EQ(expected6, *result6.payload());
}

TEST(opBuilderHelperFieldAppend, AppendToBoolean)
{
    auto tuple = std::make_tuple(targetField, helperFunctionName, vector<string> {sourceFieldRef});
    const auto op = opBuilderHelperFieldAppend(tuple);

    auto event1 = std::make_shared<Json>(R"({
        "targetField": {
            "field": false
        },
        "sourceField": {
            "field": "new_value1"
        }
    })");
    Json expected1 {R"({
        "targetField": {
            "field": "new_value1"
        },
        "sourceField": {
            "field": "new_value1"
        }
    })"};

    auto event2 = std::make_shared<Json>(R"({
        "targetField": {
            "field": false
        },
        "sourceField": {
            "field": 123
        }
    })");
    Json expected2 {R"({
        "targetField": {
            "field": 123
        },
        "sourceField": {
            "field": 123
        }
    })"};

    auto event3 = std::make_shared<Json>(R"({
        "targetField": {
            "field": false
        },
        "sourceField": {
            "field": true
        }
    })");
    Json expected3 {R"({
        "targetField": {
            "field": true
        },
        "sourceField": {
            "field": true
        }
    })"};

    auto event4 = std::make_shared<Json>(R"({
        "targetField": {
            "field": false
        },
        "sourceField": {
            "field": null
        }
    })");
    Json expected4 {R"({
        "targetField": {
            "field": null
        },
        "sourceField": {
            "field": null
        }
    })"};

    auto event5 = std::make_shared<Json>(R"({
        "targetField": {
            "field": false
        },
        "sourceField": {
            "field": {
                "subfield": "subvalue"
            }
        }
    })");
    Json expected5 {R"({
        "targetField": {
            "field": {
                "subfield": "subvalue"
            }
        },
        "sourceField": {
            "field": {
                "subfield": "subvalue"
            }
        }
    })"};

    auto event6 = std::make_shared<Json>(R"({
        "targetField": {
            "field": false
        },
        "sourceField": {
            "field": ["123", 123, false, null]
        }
    })");
    Json expected6 {R"({
        "targetField": {
            "field": ["123", 123, false, null]
        },
        "sourceField": {
            "field": ["123", 123, false, null]
        }
    })"};

    const auto result1 = op->getPtr<Term<EngineOp>>()->getFn()(event1);
    ASSERT_TRUE(result1);
    ASSERT_EQ(expected1, *result1.payload());

    const auto result2 = op->getPtr<Term<EngineOp>>()->getFn()(event2);
    ASSERT_TRUE(result2);
    ASSERT_EQ(expected2, *result2.payload());

    const auto result3 = op->getPtr<Term<EngineOp>>()->getFn()(event3);
    ASSERT_TRUE(result3);
    ASSERT_EQ(expected3, *result3.payload());

    const auto result4 = op->getPtr<Term<EngineOp>>()->getFn()(event4);
    ASSERT_TRUE(result4);
    ASSERT_EQ(expected4, *result4.payload());

    const auto result5 = op->getPtr<Term<EngineOp>>()->getFn()(event5);
    ASSERT_TRUE(result5);
    ASSERT_EQ(expected5, *result5.payload());

    const auto result6 = op->getPtr<Term<EngineOp>>()->getFn()(event6);
    ASSERT_TRUE(result6);
    ASSERT_EQ(expected6, *result6.payload());
}

TEST(opBuilderHelperFieldAppend, AppendToNull)
{
    auto tuple = std::make_tuple(targetField, helperFunctionName, vector<string> {sourceFieldRef});
    const auto op = opBuilderHelperFieldAppend(tuple);

    auto event1 = std::make_shared<Json>(R"({
        "targetField": {
            "field": null
        },
        "sourceField": {
            "field": "new_value1"
        }
    })");
    Json expected1 {R"({
        "targetField": {
            "field": "new_value1"
        },
        "sourceField": {
            "field": "new_value1"
        }
    })"};

    auto event2 = std::make_shared<Json>(R"({
        "targetField": {
            "field": null
        },
        "sourceField": {
            "field": 123
        }
    })");
    Json expected2 {R"({
        "targetField": {
            "field": 123
        },
        "sourceField": {
            "field": 123
        }
    })"};

    auto event3 = std::make_shared<Json>(R"({
        "targetField": {
            "field": null
        },
        "sourceField": {
            "field": true
        }
    })");
    Json expected3 {R"({
        "targetField": {
            "field": true
        },
        "sourceField": {
            "field": true
        }
    })"};

    auto event4 = std::make_shared<Json>(R"({
        "targetField": {
            "field": null
        },
        "sourceField": {
            "field": null
        }
    })");
    Json expected4 {R"({
        "targetField": {
            "field": null
        },
        "sourceField": {
            "field": null
        }
    })"};

    auto event5 = std::make_shared<Json>(R"({
        "targetField": {
            "field": null
        },
        "sourceField": {
            "field": {
                "subfield": "subvalue"
            }
        }
    })");
    Json expected5 {R"({
        "targetField": {
            "field": {
                "subfield": "subvalue"
            }
        },
        "sourceField": {
            "field": {
                "subfield": "subvalue"
            }
        }
    })"};

    auto event6 = std::make_shared<Json>(R"({
        "targetField": {
            "field": null
        },
        "sourceField": {
            "field": ["123", 123, false, null]
        }
    })");
    Json expected6 {R"({
        "targetField": {
            "field": ["123", 123, false, null]
        },
        "sourceField": {
            "field": ["123", 123, false, null]
        }
    })"};

    const auto result1 = op->getPtr<Term<EngineOp>>()->getFn()(event1);
    ASSERT_TRUE(result1);
    ASSERT_EQ(expected1, *result1.payload());

    const auto result2 = op->getPtr<Term<EngineOp>>()->getFn()(event2);
    ASSERT_TRUE(result2);
    ASSERT_EQ(expected2, *result2.payload());

    const auto result3 = op->getPtr<Term<EngineOp>>()->getFn()(event3);
    ASSERT_TRUE(result3);
    ASSERT_EQ(expected3, *result3.payload());

    const auto result4 = op->getPtr<Term<EngineOp>>()->getFn()(event4);
    ASSERT_TRUE(result4);
    ASSERT_EQ(expected4, *result4.payload());

    const auto result5 = op->getPtr<Term<EngineOp>>()->getFn()(event5);
    ASSERT_TRUE(result5);
    ASSERT_EQ(expected5, *result5.payload());

    const auto result6 = op->getPtr<Term<EngineOp>>()->getFn()(event6);
    ASSERT_TRUE(result6);
    ASSERT_EQ(expected6, *result6.payload());
}

TEST(opBuilderHelperFieldAppend, AppendToArray)
{
    auto tuple = std::make_tuple(targetField, helperFunctionName, vector<string> {sourceFieldRef});
    const auto op = opBuilderHelperFieldAppend(tuple);

    auto event1 = std::make_shared<Json>(R"({
        "targetField": {
            "field": [123, 12.3, "value", null, false, ["arrayvalue"], {"objkey":"objvalue"}]
        },
        "sourceField": {
            "field": "new_value"
        }
    })");
    Json expected1 {R"({
        "targetField": {
            "field": [123, 12.3, "value", null, false, ["arrayvalue"], {"objkey":"objvalue"}, "new_value"]
        },
        "sourceField": {
            "field": "new_value"
        }
    })"};

    auto event2 = std::make_shared<Json>(R"({
        "targetField": {
            "field": [123, 12.3, "value", null, false, ["arrayvalue"], {"objkey":"objvalue"}]
        },
        "sourceField": {
            "field": 789
        }
    })");
    Json expected2 {R"({
        "targetField": {
            "field": [123, 12.3, "value", null, false, ["arrayvalue"], {"objkey":"objvalue"}, 789]
        },
        "sourceField": {
            "field": 789
        }
    })"};

    auto event3 = std::make_shared<Json>(R"({
        "targetField": {
            "field": [123, 12.3, "value", null, false, ["arrayvalue"], {"objkey":"objvalue"}]
        },
        "sourceField": {
            "field": true
        }
    })");
    Json expected3 {R"({
        "targetField": {
            "field": [123, 12.3, "value", null, false, ["arrayvalue"], {"objkey":"objvalue"}, true]
        },
        "sourceField": {
            "field": true
        }
    })"};

    auto event4 = std::make_shared<Json>(R"({
        "targetField": {
            "field": [123, 12.3, "value", null, false, ["arrayvalue"], {"objkey":"objvalue"}]
        },
        "sourceField": {
            "field": null
        }
    })");
    Json expected4 {R"({
        "targetField": {
            "field": [123, 12.3, "value", null, false, ["arrayvalue"], {"objkey":"objvalue"}, null]
        },
        "sourceField": {
            "field": null
        }
    })"};

    auto event5 = std::make_shared<Json>(R"({
        "targetField": {
            "field": [123, 12.3, "value", null, false, ["arrayvalue"], {"objkey":"objvalue"}]
        },
        "sourceField": {
            "field": {
                "subfield": "subvalue"
            }
        }
    })");
    Json expected5 {R"({
        "targetField": {
            "field": [123, 12.3, "value", null, false, ["arrayvalue"], {"objkey":"objvalue"}, {"subfield": "subvalue"}]
        },
        "sourceField": {
            "field": {
                "subfield": "subvalue"
            }
        }
    })"};

    auto event6 = std::make_shared<Json>(R"({
        "targetField": {
            "field": [123, 12.3, "value", null, false, ["arrayvalue"], {"objkey":"objvalue"}]
        },
        "sourceField": {
            "field": ["123", 123, false, null]
        }
    })");
    Json expected6 {R"({
        "targetField": {
            "field": [123, 12.3, "value", null, false, ["arrayvalue"], {"objkey":"objvalue"}, ["123", 123, false, null]]
        },
        "sourceField": {
            "field": ["123", 123, false, null]
        }
    })"};

    const auto result1 = op->getPtr<Term<EngineOp>>()->getFn()(event1);
    ASSERT_TRUE(result1);
    ASSERT_EQ(expected1, *result1.payload());

    const auto result2 = op->getPtr<Term<EngineOp>>()->getFn()(event2);
    ASSERT_TRUE(result2);
    ASSERT_EQ(expected2, *result2.payload());

    const auto result3 = op->getPtr<Term<EngineOp>>()->getFn()(event3);
    ASSERT_TRUE(result3);
    ASSERT_EQ(expected3, *result3.payload());

    const auto result4 = op->getPtr<Term<EngineOp>>()->getFn()(event4);
    ASSERT_TRUE(result4);
    ASSERT_EQ(expected4, *result4.payload());

    const auto result5 = op->getPtr<Term<EngineOp>>()->getFn()(event5);
    ASSERT_TRUE(result5);
    ASSERT_EQ(expected5, *result5.payload());

    const auto result6 = op->getPtr<Term<EngineOp>>()->getFn()(event6);
    ASSERT_TRUE(result6);
    ASSERT_EQ(expected6, *result6.payload());
}

TEST(opBuilderHelperFieldAppend, AppendToJson)
{
    auto tuple = std::make_tuple(targetField, helperFunctionName, vector<string> {sourceFieldRef});
    const auto op = opBuilderHelperFieldAppend(tuple);

    auto event1 = std::make_shared<Json>(R"({
        "targetField": {
            "field": {
                "subfield": "subvalue"
            }
        },
        "sourceField": {
            "field": "new_value"
        }
    })");
    Json expected1 {R"({
        "targetField": {
            "field": "new_value"
        },
        "sourceField": {
            "field": "new_value"
        }
    })"};

    auto event2 = std::make_shared<Json>(R"({
        "targetField": {
            "field": {
                "subfield": "subvalue"
            }
        },
        "sourceField": {
            "field": 789
        }
    })");
    Json expected2 {R"({
        "targetField": {
            "field": 789
        },
        "sourceField": {
            "field": 789
        }
    })"};

    auto event3 = std::make_shared<Json>(R"({
        "targetField": {
            "field": {
                "subfield": "subvalue"
            }
        },
        "sourceField": {
            "field": true
        }
    })");
    Json expected3 {R"({
        "targetField": {
            "field": true
        },
        "sourceField": {
            "field": true
        }
    })"};

    auto event4 = std::make_shared<Json>(R"({
        "targetField": {
            "field": {
                "subfield": "subvalue"
            }
        },
        "sourceField": {
            "field": null
        }
    })");
    Json expected4 {R"({
        "targetField": {
            "field": null
        },
        "sourceField": {
            "field": null
        }
    })"};

    auto event5 = std::make_shared<Json>(R"({
        "targetField": {
            "field": {
                "subfield": "subvalue"
            }
        },
        "sourceField": {
            "field": {
                "new_subfield": "new_subvalue"
            }
        }
    })");
    Json expected5 {R"({
        "targetField": {
            "field": {
                "subfield": "subvalue",
                "new_subfield": "new_subvalue"
            }
        },
        "sourceField": {
            "field": {
                "new_subfield": "new_subvalue"
            }
        }
    })"};

    auto event6 = std::make_shared<Json>(R"({
        "targetField": {
            "field": {
                "subfield": "subvalue"
            }
        },
        "sourceField": {
            "field": ["123", 123, false, null]
        }
    })");
    Json expected6 {R"({
        "targetField": {
            "field": ["123", 123, false, null]
        },
        "sourceField": {
            "field": ["123", 123, false, null]
        }
    })"};

    const auto result1 = op->getPtr<Term<EngineOp>>()->getFn()(event1);
    ASSERT_TRUE(result1);
    ASSERT_EQ(expected1, *result1.payload());

    const auto result2 = op->getPtr<Term<EngineOp>>()->getFn()(event2);
    ASSERT_TRUE(result2);
    ASSERT_EQ(expected2, *result2.payload());

    const auto result3 = op->getPtr<Term<EngineOp>>()->getFn()(event3);
    ASSERT_TRUE(result3);
    ASSERT_EQ(expected3, *result3.payload());

    const auto result4 = op->getPtr<Term<EngineOp>>()->getFn()(event4);
    ASSERT_TRUE(result4);
    ASSERT_EQ(expected4, *result4.payload());

    const auto result5 = op->getPtr<Term<EngineOp>>()->getFn()(event5);
    ASSERT_TRUE(result5);
    ASSERT_EQ(expected5, *result5.payload());

    const auto result6 = op->getPtr<Term<EngineOp>>()->getFn()(event6);
    ASSERT_TRUE(result6);
    ASSERT_EQ(expected6, *result6.payload());
}

TEST(opBuilderHelperFieldAppend, AppendObjectsNested)
{
    auto tuple = std::make_tuple(targetField, helperFunctionName, vector<string> {sourceFieldRef});

    const auto op = opBuilderHelperFieldAppend(tuple);
    auto event = std::make_shared<Json>(R"({
        "targetField": {
            "field1": {
                "field11": "value11",
                "field12": "value12",
                "field13": {
                    "field131": "value131"
                }
            },
            "field3": {
                "field31": {
                    "field311": "value311"
                }
            }
        },
        "sourceField": {
            "field1": {
                "field12": "new_value12",
                "field13": {
                    "field131": "value131"
                },
                "field14": "value14"
            },
            "field2": {
                "field21": "value21"
            }
            "field3": {
                "field31": {
                    "field311": "new_value311",
                    "field312": "value312",
                    "field313": {
                        "field3131": "value3131"
                    }
                }
            }
        }
    })");

    Json expected {R"({
        "targetField": {
            "field1": {
                "field11": "value11",
                "field12": "new_value12",
                "field13": {
                    "field131": "value131"
                },
                "field14": "value14"
            },
            "field2": {
                "field21": "value21"
            }
            "field3": {
                "field31": {
                    "field311": "new_value311",
                    "field312": "value312",
                    "field313": {
                        "field3131": "value3131"
                    }
                }
            }
        },
        "sourceField": {
            "field1": {
                "field12": "new_value12",
                "field13": {
                    "field131": "value131"
                },
                "field14": "value14"
            },
            "field2": {
                "field21": "value21"
            }
            "field3": {
                "field31": {
                    "field311": "new_value311",
                    "field312": "value312",
                    "field313": {
                        "field3131": "value3131"
                    }
                }
            }
        }
    })"};

    const auto result = op->getPtr<Term<EngineOp>>()->getFn()(event);
    ASSERT_TRUE(result);
    ASSERT_EQ(expected, *result.payload());
}

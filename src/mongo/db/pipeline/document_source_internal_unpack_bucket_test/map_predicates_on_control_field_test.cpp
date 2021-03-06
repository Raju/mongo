/**
 *    Copyright (C) 2020-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/db/pipeline/aggregation_context_fixture.h"
#include "mongo/db/pipeline/document_source_internal_unpack_bucket.h"
#include "mongo/db/pipeline/document_source_match.h"
#include "mongo/db/pipeline/pipeline.h"
#include "mongo/db/query/util/make_data_structure.h"

namespace mongo {
namespace {

using InternalUnpackBucketPredicateMappingOptimizationTest = AggregationContextFixture;

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeMapsGTPredicatesOnControlField) {
    auto pipeline = Pipeline::parse(
        makeVector(fromjson("{$_internalUnpackBucket: {exclude: [], timeField: 'time'}}"),
                   fromjson("{$match: {a: {$gt: 1}}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT_BSONOBJ_EQ(predicate->serialize(true),
                      fromjson("{'control.max.a': {$_internalExprGt: 1}}"));
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeMapsGTEPredicatesOnControlField) {
    auto pipeline = Pipeline::parse(
        makeVector(fromjson("{$_internalUnpackBucket: {exclude: [], timeField: 'time'}}"),
                   fromjson("{$match: {a: {$gte: 1}}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT_BSONOBJ_EQ(predicate->serialize(true),
                      fromjson("{'control.max.a': {$_internalExprGte: 1}}"));
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeMapsLTPredicatesOnControlField) {
    auto pipeline = Pipeline::parse(
        makeVector(fromjson("{$_internalUnpackBucket: {exclude: [], timeField: 'time'}}"),
                   fromjson("{$match: {a: {$lt: 1}}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT_BSONOBJ_EQ(predicate->serialize(true),
                      fromjson("{'control.min.a': {$_internalExprLt: 1}}"));
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeMapsLTEPredicatesOnControlField) {
    auto pipeline = Pipeline::parse(
        makeVector(fromjson("{$_internalUnpackBucket: {exclude: [], timeField: 'time'}}"),
                   fromjson("{$match: {a: {$lte: 1}}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT_BSONOBJ_EQ(predicate->serialize(true),
                      fromjson("{'control.min.a': {$_internalExprLte: 1}}"));
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeMapsEQPredicatesOnControlField) {
    auto pipeline = Pipeline::parse(
        makeVector(fromjson("{$_internalUnpackBucket: {exclude: [], timeField: 'time'}}"),
                   fromjson("{$match: {a: {$eq: 1}}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT_BSONOBJ_EQ(predicate->serialize(true),
                      fromjson("{$and: [{'control.min.a': {$_internalExprLte: 1}}, "
                               "{'control.max.a': {$_internalExprGte: 1}}]}"));
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeMapsAndWithPushableChildrenOnControlField) {
    auto pipeline = Pipeline::parse(
        makeVector(fromjson("{$_internalUnpackBucket: {exclude: [], timeField: 'time'}}"),
                   fromjson("{$match: {$and: [{time: {$gt: 1}}, {a: {$lt: 5}}]}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT_BSONOBJ_EQ(predicate->serialize(true),
                      fromjson("{$and: [{'control.max.time': {$_internalExprGt: 1}}, "
                               "{'control.min.a': {$_internalExprLt: 5}}]}"));
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeDoesNotMapAndWithUnpushableChildrenOnControlField) {
    auto pipeline = Pipeline::parse(
        makeVector(fromjson("{$_internalUnpackBucket: {exclude: [], timeField: 'time'}}"),
                   fromjson("{$match: {$and: [{time: {$ne: 1}}, {a: {$ne: 5}}]}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT(predicate == nullptr);
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeMapsAndWithPushableAndUnpushableChildrenOnControlField) {
    auto pipeline = Pipeline::parse(
        makeVector(fromjson("{$_internalUnpackBucket: {exclude: [], timeField: 'time'}}"),
                   fromjson("{$match: {$and: [{time: {$gt: 1}}, {a: {$ne: 5}}]}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT_BSONOBJ_EQ(predicate->serialize(true),
                      fromjson("{$and: [{'control.max.time': {$_internalExprGt: 1}}]}"));
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeMapsNestedAndWithPushableChildrenOnControlField) {
    auto pipeline = Pipeline::parse(
        makeVector(
            fromjson("{$_internalUnpackBucket: {exclude: [], timeField: 'time'}}"),
            fromjson(
                "{$match: {$and: [{b: {$gte: 2}}, {$and: [{time: {$gt: 1}}, {a: {$lt: 5}}]}]}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT_BSONOBJ_EQ(predicate->serialize(true),
                      fromjson("{$and: [{'control.max.b': {$_internalExprGte: 2}}, {$and: "
                               "[{'control.max.time': {$_internalExprGt: 1}}, "
                               "{'control.min.a': {$_internalExprLt: 5}}]}]}"));
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeFurtherOptimizesNewlyAddedMatchWithSingletonAndNode) {
    auto unpackBucketObj = fromjson("{$_internalUnpackBucket: {exclude: [], timeField: 'time'}}");
    auto matchObj = fromjson("{$match: {$and: [{time: {$gt: 1}}, {a: {$ne: 5}}]}}");
    auto pipeline = Pipeline::parse(makeVector(unpackBucketObj, matchObj), getExpCtx());
    ASSERT_EQ(pipeline->getSources().size(), 2U);

    pipeline->optimizePipeline();
    ASSERT_EQ(pipeline->getSources().size(), 3U);

    auto stages = pipeline->serializeToBson();
    ASSERT_EQ(stages.size(), 3U);

    ASSERT_BSONOBJ_EQ(stages[0], fromjson("{$match: {'control.max.time': {$_internalExprGt: 1}}}"));
    ASSERT_BSONOBJ_EQ(stages[1], unpackBucketObj);
    ASSERT_BSONOBJ_EQ(stages[2], matchObj);
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeFurtherOptimizesNewlyAddedMatchWithNestedAndNodes) {
    auto unpackBucketObj = fromjson("{$_internalUnpackBucket: {exclude: [], timeField: 'time'}}");
    auto matchObj =
        fromjson("{$match: {$and: [{b: {$gte: 2}}, {$and: [{time: {$gt: 1}}, {a: {$lt: 5}}]}]}}");
    auto pipeline = Pipeline::parse(makeVector(unpackBucketObj, matchObj), getExpCtx());
    ASSERT_EQ(pipeline->getSources().size(), 2U);

    pipeline->optimizePipeline();
    ASSERT_EQ(pipeline->getSources().size(), 3U);

    auto stages = pipeline->serializeToBson();
    ASSERT_EQ(stages.size(), 3U);

    ASSERT_BSONOBJ_EQ(
        stages[0],
        fromjson("{$match: {$and: [{'control.max.b': {$_internalExprGte: 2}}, {'control.max.time': "
                 "{$_internalExprGt: 1}}, {'control.min.a': {$_internalExprLt: 5}}]}}"));
    ASSERT_BSONOBJ_EQ(stages[1], unpackBucketObj);
    ASSERT_BSONOBJ_EQ(stages[2], matchObj);
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeDoesNotMapPredicatesOnTypeObject) {
    auto pipeline = Pipeline::parse(
        makeVector(fromjson("{$_internalUnpackBucket: {exclude: [], timeField: 'time'}}"),
                   fromjson("{$match: {a: {$gt: {b: 5}}}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT(predicate == nullptr);
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeDoesNotMapPredicatesOnTypeArray) {
    auto pipeline = Pipeline::parse(
        makeVector(fromjson("{$_internalUnpackBucket: {exclude: [], timeField: 'time'}}"),
                   fromjson("{$match: {a: {$gt: [5]}}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT(predicate == nullptr);
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeDoesNotMapPredicatesOnTypeNull) {
    auto pipeline = Pipeline::parse(
        makeVector(fromjson("{$_internalUnpackBucket: {exclude: [], timeField: 'time'}}"),
                   fromjson("{$match: {a: {$gt: null}}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT(predicate == nullptr);
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeDoesNotMapMetaPredicatesOnControlField) {
    auto pipeline = Pipeline::parse(
        makeVector(
            fromjson(
                "{$_internalUnpackBucket: {exclude: [], timeField: 'time', metaField: 'myMeta'}}"),
            fromjson("{$match: {myMeta: {$gt: 5}}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT(predicate == nullptr);
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeDoesNotMapMetaPredicatesWithNestedFieldsOnControlField) {
    auto pipeline = Pipeline::parse(
        makeVector(
            fromjson(
                "{$_internalUnpackBucket: {exclude: [], timeField: 'time', metaField: 'myMeta'}}"),
            fromjson("{$match: {'myMeta.foo': {$gt: 5}}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT(predicate == nullptr);
}

TEST_F(InternalUnpackBucketPredicateMappingOptimizationTest,
       OptimizeDoesNotMapNestedMetaPredicatesOnControlField) {
    auto pipeline = Pipeline::parse(
        makeVector(
            fromjson(
                "{$_internalUnpackBucket: {exclude: [], timeField: 'time', metaField: 'myMeta'}}"),
            fromjson("{$match: {$and: [{time: {$gt: 1}}, {myMeta: {$eq: 5}}]}}")),
        getExpCtx());
    auto& container = pipeline->getSources();

    ASSERT_EQ(pipeline->getSources().size(), 2U);

    auto original = dynamic_cast<DocumentSourceMatch*>(container.back().get());
    auto predicate = dynamic_cast<DocumentSourceInternalUnpackBucket*>(container.front().get())
                         ->createPredicatesOnControlField(original->getMatchExpression());

    ASSERT_BSONOBJ_EQ(predicate->serialize(true),
                      fromjson("{$and: [{'control.max.time': {$_internalExprGt: 1}}]}"));
}

}  // namespace
}  // namespace mongo

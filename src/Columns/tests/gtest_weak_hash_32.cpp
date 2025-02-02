#include <gtest/gtest.h>

#include <Columns/ColumnsNumber.h>
#include <Columns/ColumnString.h>
#include <Columns/ColumnFixedString.h>
#include <Columns/ColumnArray.h>
#include <Columns/ColumnConst.h>
#include <Columns/ColumnDecimal.h>
#include <Columns/ColumnLowCardinality.h>
#include <Columns/ColumnNullable.h>
#include <Columns/ColumnTuple.h>

#include <DataTypes/DataTypeLowCardinality.h>
#include <DataTypes/DataTypesNumber.h>

#include <Common/WeakHash.h>
#include <Common/hex.h>

#include <unordered_map>
#include <iostream>
#include <sstream>


using namespace DB;

template <typename T>
void checkColumn(
    const WeakHash32::Container & hash,
    const PaddedPODArray<T> & eq_class,
    size_t allowed_collisions = 0,
    size_t max_collisions_to_print = 10)
{
    ASSERT_EQ(hash.size(), eq_class.size());

    /// Check equal rows has equal hash.
    {
        std::unordered_map<T, UInt32> map;

        for (size_t i = 0; i < eq_class.size(); ++i)
        {
            auto & val = eq_class[i];
            auto it = map.find(val);

            if (it == map.end())
                map[val] = hash[i];
            else
            {
                if (it->second != hash[i])
                    std::cout << "Different hashes for the same equivalent class (" << toString(val) << ")\n";

                ASSERT_EQ(it->second, hash[i]);
            }
        }
    }

    /// Check have not many collisions.
    {
        std::unordered_map<UInt32, T> map;
        size_t num_collisions = 0;

        std::stringstream collisions_str;       // STYLE_CHECK_ALLOW_STD_STRING_STREAM
        collisions_str.exceptions(std::ios::failbit);

        for (size_t i = 0; i < eq_class.size(); ++i)
        {
            auto & val = eq_class[i];
            auto it = map.find(hash[i]);

            if (it == map.end())
                map[hash[i]] = val;
            else if (it->second != val)
            {
                ++num_collisions;

                if (num_collisions <= max_collisions_to_print)
                {
                    collisions_str << "Collision:\n";
                }

                if (num_collisions > allowed_collisions)
                {
                    std::cerr << collisions_str.rdbuf();
                    break;
                }
            }
        }

        ASSERT_LE(num_collisions, allowed_collisions);
    }
}

TEST(WeakHash32, ColumnVectorU8)
{
    auto col = ColumnUInt8::create();
    auto & data = col->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (size_t i = 0; i < 265; ++i)
            data.push_back(i);
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), col->getData());
}

TEST(WeakHash32, ColumnVectorI8)
{
    auto col = ColumnInt8::create();
    auto & data = col->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (int i = -128; i < 128; ++i)
            data.push_back(i);
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), col->getData());
}

TEST(WeakHash32, ColumnVectorU16)
{
    auto col = ColumnUInt16::create();
    auto & data = col->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (size_t i = 0; i < 65536; ++i)
            data.push_back(i);
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), col->getData());
}

TEST(WeakHash32, ColumnVectorI16)
{
    auto col = ColumnInt16::create();
    auto & data = col->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (int i = -32768; i < 32768; ++i)
            data.push_back(i);
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), col->getData());
}

TEST(WeakHash32, ColumnVectorU32)
{
    auto col = ColumnUInt32::create();
    auto & data = col->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (uint32_t i = 0; i < 65536; ++i)
            data.push_back(i << 16u);
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), col->getData());
}

TEST(WeakHash32, ColumnVectorI32)
{
    auto col = ColumnInt32::create();
    auto & data = col->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (int32_t i = -32768; i < 32768; ++i)
            data.push_back(i << 16);
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), col->getData());
}

TEST(WeakHash32, ColumnVectorU64)
{
    auto col = ColumnUInt64::create();
    auto & data = col->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (uint64_t i = 0; i < 65536; ++i)
            data.push_back(i << 32u);
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), col->getData());
}

TEST(WeakHash32, ColumnVectorI64)
{
    auto col = ColumnInt64::create();
    auto & data = col->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (int64_t i = -32768; i < 32768; ++i)
            data.push_back(i << 32);
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), col->getData());
}

TEST(WeakHash32, ColumnVectorU128)
{
    auto col = ColumnUInt128::create();
    auto & data = col->getData();
    auto eq = ColumnUInt32::create();
    auto & eq_data = eq->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (uint64_t i = 0; i < 65536; ++i)
        {
            UInt128 val;
            val.items[0] = i << 32u;
            val.items[1] = i << 32u;
            data.push_back(val);
            eq_data.push_back(static_cast<UInt32>(i));
        }
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), eq_data);
}

TEST(WeakHash32, ColumnVectorI128)
{
    auto col = ColumnInt128::create();
    auto & data = col->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (int64_t i = -32768; i < 32768; ++i)
            data.push_back(i << 32);
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), col->getData());
}

TEST(WeakHash32, ColumnDecimal32)
{
    auto col = ColumnDecimal<Decimal32>::create(0, 0);
    auto & data = col->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (int32_t i = -32768; i < 32768; ++i)
            data.push_back(i << 16);
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), col->getData());
}

TEST(WeakHash32, ColumnDecimal64)
{
    auto col = ColumnDecimal<Decimal64>::create(0, 0);
    auto & data = col->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (int64_t i = -32768; i < 32768; ++i)
            data.push_back(i << 32);
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), col->getData());
}

TEST(WeakHash32, ColumnDecimal128)
{
    auto col = ColumnDecimal<Decimal128>::create(0, 0);
    auto & data = col->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (int64_t i = -32768; i < 32768; ++i)
            data.push_back(i << 32);
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), col->getData());
}

TEST(WeakHash32, ColumnString1)
{
    auto col = ColumnString::create();
    auto eq = ColumnUInt32::create();
    auto & data = eq->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (int32_t i = 0; i < 65536; ++i)
        {
            data.push_back(i);
            auto str = std::to_string(i);
            col->insertData(str.data(), str.size());
        }
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), data);
}

TEST(WeakHash32, ColumnString2)
{
    auto col = ColumnString::create();
    auto eq = ColumnUInt32::create();
    auto & data = eq->getData();

    /*
     * a
     * aa
     * aaa
     * ...
     * b
     * bb
     * bbb
     */
    for (int idx [[maybe_unused]] : {1, 2})
    {
        size_t max_size = 3000;
        char letter = 'a';
        for (int32_t i = 0; i < 65536; ++i)
        {
            data.push_back(i);
            size_t s = (i % max_size) + 1;
            std::string str(s, letter);
            col->insertData(str.data(), str.size());

            if (s == max_size)
                ++letter;
        }
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    /// Now there is single collision between 'k' * 544 and 'q' * 2512 (which is calculated twice)
    size_t allowed_collisions = 4;

    checkColumn(hash.getData(), data, allowed_collisions);
}

TEST(WeakHash32, ColumnString3)
{
    auto col = ColumnString::create();
    auto eq = ColumnUInt32::create();
    auto & data = eq->getData();

    /*
     * a
     * a\0
     * a\0\0
     * ...
     * b
     * b\0
     * b\0\0
    */
    for (int idx [[maybe_unused]] : {1, 2})
    {
        size_t max_size = 3000;
        char letter = 'a';
        for (int64_t i = 0; i < 65536; ++i)
        {
            data.push_back(static_cast<UInt32>(i));
            size_t s = (i % max_size) + 1;
            std::string str(s,'\0');
            str[0] = letter;
            col->insertData(str.data(), str.size());

            if (s == max_size)
                ++letter;
        }
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), data);
}

TEST(WeakHash32, ColumnFixedString)
{
    size_t max_size = 3000;
    auto col = ColumnFixedString::create(max_size);
    auto eq = ColumnUInt32::create();
    auto & data = eq->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        char letter = 'a';
        for (int64_t i = 0; i < 65536; ++i)
        {
            data.push_back(static_cast<UInt32>(i));
            size_t s = (i % max_size) + 1;
            std::string str(s, letter);
            col->insertData(str.data(), str.size());

            if (s == max_size)
                ++letter;
        }
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), data);
}

TEST(WeakHash32, ColumnArray)
{
    size_t max_size = 3000;
    auto val = ColumnUInt32::create();
    auto off = ColumnUInt64::create();
    auto eq = ColumnUInt32::create();
    auto & eq_data = eq->getData();
    auto & val_data = val->getData();
    auto & off_data = off->getData();

    /* [1]
     * [1, 1]
     * [1, 1, 1]
     * ...
     * [2]
     * [2, 2]
     * [2, 2, 2]
     * ...
     */
    UInt64 cur_off = 0;
    for (int idx [[maybe_unused]] : {1, 2})
    {
        UInt32 cur = 0;
        for (int64_t i = 0; i < 65536; ++i)
        {
            eq_data.push_back(static_cast<UInt32>(i));
            size_t s = (i % max_size) + 1;

            cur_off += s;
            off_data.push_back(cur_off);

            for (size_t j = 0; j < s; ++j)
                val_data.push_back(cur);

            if (s == max_size)
                ++cur;
        }
    }

    auto col_arr = ColumnArray::create(std::move(val), std::move(off));

    WeakHash32 hash(col_arr->size());
    col_arr->updateWeakHash32(hash);

    checkColumn(hash.getData(), eq_data);
}

TEST(WeakHash32, ColumnArray2)
{
    auto val = ColumnUInt32::create();
    auto off = ColumnUInt64::create();
    auto eq = ColumnUInt32::create();
    auto & eq_data = eq->getData();
    auto & val_data = val->getData();
    auto & off_data = off->getData();

    UInt64 cur_off = 0;
    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (int32_t i = 0; i < 1000; ++i)
        {
            for (uint32_t j = 0; j < 1000; ++j)
            {
                eq_data.push_back(i * 1000 + j);

                cur_off += 2;
                off_data.push_back(cur_off);

                val_data.push_back(i);
                val_data.push_back(j);
            }
        }
    }

    auto col_arr = ColumnArray::create(std::move(val), std::move(off));

    WeakHash32 hash(col_arr->size());
    col_arr->updateWeakHash32(hash);

    checkColumn(hash.getData(), eq_data);
}

TEST(WeakHash32, ColumnArrayArray)
{
    size_t max_size = 3000;
    auto val = ColumnUInt32::create();
    auto off = ColumnUInt64::create();
    auto off2 = ColumnUInt64::create();
    auto eq = ColumnUInt32::create();
    auto & eq_data = eq->getData();
    auto & val_data = val->getData();
    auto & off_data = off->getData();
    auto & off2_data = off2->getData();

    /* [[0]]
     * [[0], [0]]
     * [[0], [0], [0]]
     * ...
     * [[0, 0]]
     * [[0, 0], [0, 0]]
     * [[0, 0], [0, 0], [0, 0]]
     * ...
     */
    UInt64 cur_off = 0;
    UInt64 cur_off2 = 0;
    for (int idx [[maybe_unused]] : {1, 2})
    {
        UInt32 cur = 1;
        for (int64_t i = 0; i < 3000; ++i)
        {
            eq_data.push_back(static_cast<UInt32>(i));
            size_t s = (i % max_size) + 1;

            cur_off2 += s;
            off2_data.push_back(cur_off2);

            for (size_t j = 0; j < s; ++j)
            {
                for (size_t k = 0; k < cur; ++k)
                    val_data.push_back(0);

                cur_off += cur;
                off_data.push_back(cur_off);
            }

            if (s == max_size)
                ++cur;
        }
    }

    auto col_arr = ColumnArray::create(std::move(val), std::move(off));
    auto col_arr_arr = ColumnArray::create(std::move(col_arr), std::move(off2));

    WeakHash32 hash(col_arr_arr->size());
    col_arr_arr->updateWeakHash32(hash);

    checkColumn(hash.getData(), eq_data);
}

TEST(WeakHash32, ColumnConst)
{
    auto inner_col = ColumnUInt8::create();
    inner_col->insert(0);

    auto cls = ColumnUInt8::create();
    auto & data = cls->getData();

    for (size_t i = 0; i < 256; ++i)
        data.push_back(0);

    auto col_const = ColumnConst::create(std::move(inner_col), 256);

    WeakHash32 hash(col_const->size());
    col_const->updateWeakHash32(hash);

    checkColumn(hash.getData(), data);
}

TEST(WeakHash32, ColumnLowcardinality)
{
    auto col = DataTypeLowCardinality(std::make_shared<DataTypeUInt8>()).createColumn();
    auto eq = ColumnUInt8::create();
    auto & data = eq->getData();

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (size_t i = 0; i < 65536; ++i)
        {
            data.push_back(i);
            col->insert(i);
        }
    }

    WeakHash32 hash(col->size());
    col->updateWeakHash32(hash);

    checkColumn(hash.getData(), data);
}

TEST(WeakHash32, ColumnNullable)
{
    auto col = ColumnUInt64::create();
    auto & data = col->getData();
    auto mask = ColumnUInt8::create();
    auto & mask_data = mask->getData();
    PaddedPODArray<Int64> eq;

    for (int idx [[maybe_unused]] : {1, 2})
    {
        for (uint64_t i = 0; i < 65536; ++i)
        {
            data.push_back(i << 32u);
            mask_data.push_back(i % 7 == 0 ? 1 : 0);
            eq.push_back(i % 7 == 0 ? -1 : (i << 32u));
        }
    }

    auto col_null = ColumnNullable::create(std::move(col), std::move(mask));

    WeakHash32 hash(col_null->size());
    col_null->updateWeakHash32(hash);

    checkColumn(hash.getData(), eq);
}

TEST(WeakHash32, ColumnTupleUInt64UInt64)
{
    auto col1 = ColumnUInt64::create();
    auto col2 = ColumnUInt64::create();
    auto & data1 = col1->getData();
    auto & data2 = col2->getData();
    PaddedPODArray<Int32> eq;

    for (int idx : {0, 1, 2, 3})
    {
        auto l = idx % 2;

        for (uint64_t i = 0; i < 65536; ++i)
        {
            data1.push_back(l);
            data2.push_back(i << 32u);
            eq.push_back(static_cast<UInt32>(l * 65536 + i));
        }
    }

    Columns columns;
    columns.emplace_back(std::move(col1));
    columns.emplace_back(std::move(col2));
    auto col_tuple = ColumnTuple::create(std::move(columns));

    WeakHash32 hash(col_tuple->size());
    col_tuple->updateWeakHash32(hash);

    checkColumn(hash.getData(), eq);
}

TEST(WeakHash32, ColumnTupleUInt64String)
{
    auto col1 = ColumnUInt64::create();
    auto col2 = ColumnString::create();
    auto & data1 = col1->getData();
    PaddedPODArray<Int32> eq;

    for (int idx : {0, 1, 2, 3})
    {
        auto l = idx % 2;

        size_t max_size = 3000;
        char letter = 'a';
        for (int32_t i = 0; i < 65536; ++i)
        {
            data1.push_back(l);
            eq.push_back(l * 65536 + i);

            size_t s = (i % max_size) + 1;
            std::string str(s, letter);
            col2->insertData(str.data(), str.size());

            if (s == max_size)
                ++letter;
        }
    }

    Columns columns;
    columns.emplace_back(std::move(col1));
    columns.emplace_back(std::move(col2));
    auto col_tuple = ColumnTuple::create(std::move(columns));

    WeakHash32 hash(col_tuple->size());
    col_tuple->updateWeakHash32(hash);

    size_t allowed_collisions = 8;
    checkColumn(hash.getData(), eq, allowed_collisions);
}

TEST(WeakHash32, ColumnTupleUInt64FixedString)
{
    size_t max_size = 3000;
    auto col1 = ColumnUInt64::create();
    auto col2 = ColumnFixedString::create(max_size);
    auto & data1 = col1->getData();
    PaddedPODArray<Int32> eq;

    for (int idx : {0, 1, 2, 3})
    {
        auto l = idx % 2;

        char letter = 'a';
        for (int64_t i = 0; i < 65536; ++i)
        {
            data1.push_back(l);
            eq.push_back(static_cast<Int32>(l * 65536 + i));

            size_t s = (i % max_size) + 1;
            std::string str(s, letter);
            col2->insertData(str.data(), str.size());

            if (s == max_size)
                ++letter;
        }
    }

    Columns columns;
    columns.emplace_back(std::move(col1));
    columns.emplace_back(std::move(col2));
    auto col_tuple = ColumnTuple::create(std::move(columns));

    WeakHash32 hash(col_tuple->size());
    col_tuple->updateWeakHash32(hash);

    checkColumn(hash.getData(), eq);
}

TEST(WeakHash32, ColumnTupleUInt64Array)
{
    size_t max_size = 3000;
    auto val = ColumnUInt32::create();
    auto off = ColumnUInt64::create();
    auto eq = ColumnUInt32::create();
    auto & eq_data = eq->getData();
    auto & val_data = val->getData();
    auto & off_data = off->getData();

    auto col1 = ColumnUInt64::create();
    auto & data1 = col1->getData();

    UInt64 cur_off = 0;
    for (int idx : {0, 1, 2, 3})
    {
        auto l = idx % 2;

        UInt32 cur = 0;
        for (int32_t i = 0; i < 65536; ++i)
        {
            data1.push_back(l);
            eq_data.push_back(l * 65536 + i);
            size_t s = (i % max_size) + 1;

            cur_off += s;
            off_data.push_back(cur_off);

            for (size_t j = 0; j < s; ++j)
                val_data.push_back(cur);

            if (s == max_size)
                ++cur;
        }
    }

    Columns columns;
    columns.emplace_back(std::move(col1));
    columns.emplace_back(ColumnArray::create(std::move(val), std::move(off)));
    auto col_tuple = ColumnTuple::create(std::move(columns));

    WeakHash32 hash(col_tuple->size());
    col_tuple->updateWeakHash32(hash);

    /// There are 2 collisions right now (repeated 2 times each):
    /// (0, [array of size 1212 with values 7]) vs (0, [array of size 2265 with values 17])
    /// (0, [array of size 558 with values 5]) vs (1, [array of size 879 with values 21])

    size_t allowed_collisions = 8;
    checkColumn(hash.getData(), eq_data, allowed_collisions);
}

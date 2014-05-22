#!/usr/bin/env perl
use strict;

if(@ARGV != 1)
{
    print "Usage $0 <test_case_file_to_transfer>\n";
    exit;
}

my $infile = shift @ARGV;

if(!open (IN, "<$infile"))
{
    print "Can't open input file: $infile\n";
    exit;
}

my $enter_main_func = 0;

while(<IN>)
{
    my $x = $_;

    if ($x =~ /#include(.*?)(unit_test|UnitTest).hpp/)
    {
        $x = "#include <gtest/gtest.h>\n";
    }

    if ($x =~ /UNIT_TEST_CASE\((.*?)\)/) 
    {
        my $t = "TEST($1Test, $1)";
        $x =~ s/UNIT_TEST_CASE\($1\)/$t/g;
    }

    if ($x =~ /(int|void)?[ ]*main[ ]*\((.*?)\)/)
    {
        $enter_main_func = 1;
        my $t = "$1 main \(int argc, char** argv\)";
        $x =~ s/(int|void)?[ ]*main[ ]*\((.*?)\)/$t/g;
    }

    $x =~ s/UNIT_TEST_REQUIRE_EQ/ASSERT_EQ/g;
    $x =~ s/UNIT_TEST_REQUIRE_NE/ASSERT_NE/g;
    $x =~ s/UNIT_TEST_REQUIRE_LT/ASSERT_LT/g;
    $x =~ s/UNIT_TEST_REQUIRE_LE/ASSERT_LE/g;
    $x =~ s/UNIT_TEST_REQUIRE_GT/ASSERT_GT/g;
    $x =~ s/UNIT_TEST_REQUIRE_GE/ASSERT_GE/g;
    $x =~ s/UNIT_TEST_REQUIRE/ASSERT_TRUE/g;

    $x =~ s/UNIT_TEST_EXPECT_EQ/EXPECT_EQ/g;
    $x =~ s/UNIT_TEST_EXPECT_NE/EXPECT_NE/g;
    $x =~ s/UNIT_TEST_EXPECT_LT/EXPECT_LT/g;
    $x =~ s/UNIT_TEST_EXPECT_LE/EXPECT_LE/g;
    $x =~ s/UNIT_TEST_EXPECT_GT/EXPECT_GT/g;
    $x =~ s/UNIT_TEST_EXPECT_GE/EXPECT_GE/g;
    $x =~ s/UNIT_TEST_EXPECT/EXPECT_TRUE/g;

    $x =~ s/UNIT_TEST_CHECK_EQ/EXPECT_EQ/g;
    $x =~ s/UNIT_TEST_CHECK_NE/EXPECT_NE/g;
    $x =~ s/UNIT_TEST_CHECK_LT/EXPECT_LT/g;
    $x =~ s/UNIT_TEST_CHECK_LE/EXPECT_LE/g;
    $x =~ s/UNIT_TEST_CHECK_GT/EXPECT_GT/g;
    $x =~ s/UNIT_TEST_CHECK_GE/EXPECT_GE/g;
    $x =~ s/UNIT_TEST_CHECK/EXPECT_TRUE/g;

    $x =~ s/UNIT_TEST_RUN/RUN_ALL_TESTS/g;

    print $x;
    if ($enter_main_func and $x =~ /\{/)
    {
        print "    testing::InitGoogleTest(&argc, argv);\n";
        $enter_main_func = 0;
    }
}
close(IN);



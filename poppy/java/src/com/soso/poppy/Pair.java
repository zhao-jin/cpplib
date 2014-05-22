/*
 * @(#)Pair.java	
 * 
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

import java.util.AbstractMap;
import java.lang.Comparable;

public class Pair<A extends Comparable<A>, B extends Comparable<B>> extends
        AbstractMap.SimpleEntry<A, B> implements Comparable<Pair<A, B>> {
    private static final long serialVersionUID = 1L;

    public Pair(A a, B b) {
        super(a, b);
    }

    public int compareTo(Pair<A, B> rhs) {
        int result = this.getKey().compareTo(rhs.getKey());
        if (result != 0) {
            return result;
        }
        return this.getValue().compareTo(rhs.getValue());
    }
}

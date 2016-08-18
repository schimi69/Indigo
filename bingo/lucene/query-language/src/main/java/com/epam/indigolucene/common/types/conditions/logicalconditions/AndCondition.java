package com.epam.indigolucene.common.types.conditions.logicalconditions;

import com.epam.indigolucene.common.types.conditions.AbstractCondition;
import com.epam.indigolucene.common.types.conditions.BinaryCondition;
import com.epam.indigolucene.common.types.conditions.Condition;
import org.json.simple.JSONObject;

/**
 * Created by Artem Malykh on 20.02.16.
 * Represents 'and' condition
 */
public class AndCondition<S> extends BinaryCondition<S> {
    public static final String OP_AND = "OP_AND";
    public static final String SOLR_AND = "AND";

    public AndCondition(Condition<S> c1, Condition<S> c2) {
        super(c1, c2);
    }

    @Override
    public String operationName() {
        return OP_AND;
    }

    @Override
    public Condition<S> not() {
        return new OrCondition<>(getFirstOperand().not(), getSecondOperand().not());
    }

    @Override
    public String solrOperationName() {
        return SOLR_AND;
    }

    public static <S> Condition<S> andFromJson(JSONObject json) {
        return binaryOperatorFromJson(json, AbstractCondition::fromJson, AndCondition::new);
    }
}

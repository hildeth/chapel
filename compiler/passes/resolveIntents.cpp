/*
 * Copyright 2004-2015 Cray Inc.
 * Other additional copyright holders may be indicated within.
 * 
 * The entirety of this work is licensed under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License.
 * 
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "resolveIntents.h"

#include "passes.h"
#include "astutil.h"
#include "stlUtil.h"
#include "expr.h"

bool intentsResolved = false;

static IntentTag constIntentForType(Type* t) {
  if (isSyncType(t) ||
      isRecordWrappedType(t) ||  // domain, array, or distribution
      isRecord(t)) { // may eventually want to decide based on size
    return INTENT_CONST_REF;
  } else if (is_bool_type(t) ||
             is_int_type(t) ||
             is_uint_type(t) ||
             is_real_type(t) ||
             is_imag_type(t) ||
             is_complex_type(t) ||
             is_enum_type(t) ||
             isClass(t) ||
             isUnion(t) ||
             isAtomicType(t) ||
             t == dtOpaque ||
             t == dtTaskID ||
             t == dtFile ||
             t == dtTaskList ||
             t == dtNil ||
             t == dtStringC ||
             t == dtStringCopy ||
             t->symbol->hasFlag(FLAG_EXTERN)) {
    return INTENT_CONST_IN;
  }
  INT_FATAL(t, "Unhandled type in constIntentForType()");
  return INTENT_CONST;
}

IntentTag blankIntentForType(Type* t) {
  if (isSyncType(t) ||
      isAtomicType(t) ||
      t->symbol->hasFlag(FLAG_ARRAY)) {
    return INTENT_REF;
  } else if (is_bool_type(t) ||
             is_int_type(t) ||
             is_uint_type(t) ||
             is_real_type(t) ||
             is_imag_type(t) ||
             is_complex_type(t) ||
             is_enum_type(t) ||
             t == dtStringC ||
             t == dtStringCopy ||
             isClass(t) ||
             isRecord(t) ||
             isUnion(t) ||
             t == dtTaskID ||
             t == dtFile ||
             t == dtTaskList ||
             t == dtNil ||
             t == dtOpaque ||
             t->symbol->hasFlag(FLAG_DOMAIN) ||
             t->symbol->hasFlag(FLAG_DISTRIBUTION) ||
             t->symbol->hasFlag(FLAG_EXTERN)) {
    return constIntentForType(t);
  }
  INT_FATAL(t, "Unhandled type in blankIntentForType()");
  return INTENT_BLANK;
}

IntentTag concreteIntent(IntentTag existingIntent, Type* t) {
  if (existingIntent == INTENT_BLANK) {
    return blankIntentForType(t);
  } else if (existingIntent == INTENT_CONST) {
    return constIntentForType(t);
  } else {
    return existingIntent;
  }
}

static IntentTag blankIntentForThisArg(Type* t) {
  // todo: be honest when 't' is an array or domain
  return INTENT_CONST_IN;
}

void resolveArgIntent(ArgSymbol* arg) {
  arg->intent = 
    arg->hasFlag(FLAG_ARG_THIS) && arg->intent == INTENT_BLANK ?
    blankIntentForThisArg(arg->type) :
    concreteIntent(arg->intent, arg->type);
}

static inline void insertDerefTmp(SymExpr* se)
{
  SET_LINENO(se);

  ArgSymbol* arg = toArgSymbol(se->var);
  VarSymbol* tmp = newTemp("deref_tmp", arg->type->getValType());
  CallExpr* call = toCallExpr(se->parentExpr);
  Expr* stmt = call->getStmtExpr();
  stmt->insertBefore(new DefExpr(tmp));
  stmt->insertBefore(new CallExpr(PRIM_MOVE, tmp,
                                  new CallExpr(PRIM_DEREF, arg)));
  se->var = tmp;
}

static void convertValArgToRefArg(ArgSymbol* arg)
{
  arg->type = arg->type->getRefType();

  // Collect all the se's in the function.
  std::vector<SymExpr*> symExprs;
  collectSymExprs(arg->defPoint->parentSymbol, symExprs);
  for_vector(SymExpr, se, symExprs)
  {
    if (se->var != arg)
      continue;

    if (CallExpr* call = toCallExpr(se->parentExpr))
    {
      if (call->isResolved())
      {
        ArgSymbol* formal = actual_to_formal(se);
        if (! formal->type->symbol->hasFlag(FLAG_REF))
          insertDerefTmp(se);
      }
      else
      {
        if (call->isPrimitive(PRIM_ADDR_OF))
          call->replace(se);
        else
          insertDerefTmp(se);
      }
    }
  }
}

// Replace the arg type with its ref type except for special cases.
static void maybeUseRefType(ArgSymbol* arg)
{
  Type* t = arg->type;

  // Right now, only do this for the string type.
  // Eventually, we want to do this for all non-POD record types.
  if (t != dtString)
    return;

  INT_ASSERT(arg->type != arg->type->getRefType());

  convertValArgToRefArg(arg);
}

void resolveIntents() {
  forv_Vec(ArgSymbol, arg, gArgSymbols) {
    IntentTag oldIntent = arg->intent;
    resolveArgIntent(arg);
    if ((arg->intent ^ oldIntent) & INTENT_FLAG_REF)
    {
      // The ref flag changed.
      maybeUseRefType(arg);
    }
  }
  intentsResolved = true;
}

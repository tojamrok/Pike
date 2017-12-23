
#line 1 "rl/json5_mapping.rl"
/* vim:syntax=ragel
 */


#line 83 "rl/json5_mapping.rl"


static ptrdiff_t _parse_JSON5_mapping(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    /* GCC complains about a being used uninitialized. This is clearly wrong, so
     * lets silence this warning */
    struct mapping *m = m;
    int cs;
    int c = 0;
    struct string_builder s;
    ptrdiff_t eof = pe;
    ptrdiff_t identifier_start;

    ONERROR handle;
    const int validate = !(state->flags&JSON5_VALIDATE);

    
#line 25 "json5_mapping.c"
static const int JSON5_mapping_start = 1;
static const int JSON5_mapping_first_final = 23;
static const int JSON5_mapping_error = 0;

static const int JSON5_mapping_en_main = 1;


#line 99 "rl/json5_mapping.rl"

    /* Check stacks since we have uncontrolled recursion here. */
    check_stack (10);
    check_c_stack (1024);

    if (validate) {
	m = debug_allocate_mapping(5);
	push_mapping(m);
    }

    
#line 45 "json5_mapping.c"
	{
	cs = JSON5_mapping_start;
	}

#line 110 "rl/json5_mapping.rl"
    
#line 52 "json5_mapping.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 123 )
		goto st2;
	goto st0;
st0:
cs = 0;
	goto _out;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto st2;
		case 32: goto st2;
		case 34: goto tr2;
		case 47: goto st18;
		case 95: goto tr4;
		case 125: goto tr5;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
			goto st2;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 90 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 122 )
			goto tr4;
	} else
		goto tr4;
	goto st0;
tr2:
#line 33 "rl/json5_mapping.rl"
	{
printf("parse_key\n");
	state->level++;
	if (state->flags&JSON5_UTF8)
	    p = _parse_JSON5_string_utf8(str, p, pe, state);
	else
	    p = _parse_JSON5_string(str, p, pe, state);
	state->level--;

	if (state->flags&JSON5_ERROR) {
	    if (validate) {
		pop_stack(); /* pop mapping */
	    }
	    return p;
	}

	c++;
	{p = (( p))-1;}
    }
	goto st3;
tr26:
#line 58 "rl/json5_mapping.rl"
	{
      ptrdiff_t len = p - identifier_start;
      printf("identifier_end\n");
      if (validate) {
       init_string_builder(&s, 0);
       string_builder_append(&s, ADD_PCHARP(str, identifier_start), len);
        push_string(finish_string_builder(&s));
        UNSET_ONERROR(handle);
      }
    }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 125 "json5_mapping.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto st3;
		case 32: goto st3;
		case 47: goto st4;
		case 58: goto st8;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto st3;
	goto st0;
tr27:
#line 58 "rl/json5_mapping.rl"
	{
      ptrdiff_t len = p - identifier_start;
      printf("identifier_end\n");
      if (validate) {
       init_string_builder(&s, 0);
       string_builder_append(&s, ADD_PCHARP(str, identifier_start), len);
        push_string(finish_string_builder(&s));
        UNSET_ONERROR(handle);
      }
    }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 152 "json5_mapping.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st5;
		case 47: goto st7;
	}
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 42 )
		goto st6;
	goto st5;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st6;
		case 47: goto st3;
	}
	goto st5;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 10 )
		goto st3;
	goto st7;
tr29:
#line 58 "rl/json5_mapping.rl"
	{
      ptrdiff_t len = p - identifier_start;
      printf("identifier_end\n");
      if (validate) {
       init_string_builder(&s, 0);
       string_builder_append(&s, ADD_PCHARP(str, identifier_start), len);
        push_string(finish_string_builder(&s));
        UNSET_ONERROR(handle);
      }
    }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 198 "json5_mapping.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto st8;
		case 32: goto st8;
		case 34: goto tr12;
		case 39: goto tr12;
		case 43: goto tr12;
		case 47: goto st14;
		case 73: goto tr12;
		case 78: goto tr12;
		case 91: goto tr12;
		case 102: goto tr12;
		case 110: goto tr12;
		case 116: goto tr12;
		case 123: goto tr12;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 45 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr12;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto st8;
	goto st0;
tr12:
#line 10 "rl/json5_mapping.rl"
	{
printf("parse_value\n");
	state->level++;
	p = _parse_JSON5(str, p, pe, state);
	state->level--;

	if (state->flags&JSON5_ERROR) {
printf("got error parsing value.\n");
	    if (validate) {
		pop_2_elems(); /* pop mapping and key */
	    }
	    return p;
	} else if (validate) {
printf("inserting value into mapping.\n");
	    mapping_insert(m, &(Pike_sp[-2]), &(Pike_sp[-1]));
	    pop_2_elems();
printf("popped values.\n");
	}

	c++;
	{p = (( p))-1;}
    }
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 249 "json5_mapping.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto st9;
		case 32: goto st9;
		case 44: goto st2;
		case 47: goto st10;
		case 125: goto tr16;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto st9;
	goto st0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st11;
		case 47: goto st13;
	}
	goto st0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 42 )
		goto st12;
	goto st11;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st12;
		case 47: goto st9;
	}
	goto st11;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 10 )
		goto st9;
	goto st13;
tr5:
#line 71 "rl/json5_mapping.rl"
	{printf("final1\n");}
	goto st23;
tr16:
#line 80 "rl/json5_mapping.rl"
	{printf("final\n");}
	goto st23;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
#line 82 "rl/json5_mapping.rl"
	{ p--; {p++; cs = 23; goto _out;} }
#line 306 "json5_mapping.c"
	goto st0;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st15;
		case 47: goto st17;
	}
	goto st0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 42 )
		goto st16;
	goto st15;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st16;
		case 47: goto st8;
	}
	goto st15;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 10 )
		goto st8;
	goto st17;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st19;
		case 47: goto st21;
	}
	goto st0;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 42 )
		goto st20;
	goto st19;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st20;
		case 47: goto st2;
	}
	goto st19;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 10 )
		goto st2;
	goto st21;
tr4:
#line 53 "rl/json5_mapping.rl"
	{
      identifier_start = p;
      printf("identifier_start\n");
    }
	goto st22;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
#line 383 "json5_mapping.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr26;
		case 32: goto tr26;
		case 47: goto tr27;
		case 58: goto tr29;
		case 95: goto st22;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) < 48 ) {
		if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
			goto tr26;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 57 ) {
		if ( ( ((int)INDEX_PCHARP(str, p))) > 90 ) {
			if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 122 )
				goto st22;
		} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 65 )
			goto st22;
	} else
		goto st22;
	goto st0;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 
	_test_eof11: cs = 11; goto _test_eof; 
	_test_eof12: cs = 12; goto _test_eof; 
	_test_eof13: cs = 13; goto _test_eof; 
	_test_eof23: cs = 23; goto _test_eof; 
	_test_eof14: cs = 14; goto _test_eof; 
	_test_eof15: cs = 15; goto _test_eof; 
	_test_eof16: cs = 16; goto _test_eof; 
	_test_eof17: cs = 17; goto _test_eof; 
	_test_eof18: cs = 18; goto _test_eof; 
	_test_eof19: cs = 19; goto _test_eof; 
	_test_eof20: cs = 20; goto _test_eof; 
	_test_eof21: cs = 21; goto _test_eof; 
	_test_eof22: cs = 22; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 111 "rl/json5_mapping.rl"
printf("cs: %d\n", cs);
    if (cs >= JSON5_mapping_first_final) {
	return p;
    }
printf("got error\n");
    state->flags |= JSON5_ERROR;
    if (validate) {
	if (c & 1) pop_2_elems(); /* pop key and mapping */
	else pop_stack();
    }

    return p;
}


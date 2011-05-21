<?php

    define("BYTEKIT_IS_SAFE", 1);
    define("BYTEKIT_IS_UNSAFE", 2);

    $fname = $_SERVER['argv'][1];

    $res = bytekit_disassemble_file($fname);

    /* Search in all functions */
    $found = array();
    foreach ($res['functions'] as $name => &$func) {
        $r = &$func['raw'];
        $c = &$func['code'];
        $o = &$r['opcodes'];

        /* Find all INCLUDE_OR_EVAL opcodes */
        for ($i=0; $i<count($o); $i++) {
            if ($o[$i]['opcode'] == BYTEKIT_INCLUDE_OR_EVAL) {

                /* Find the first opline of this BB */
                $end = $start = $i;
                $start_bb = bytekit_opline_to_bb($func, $i);
                while ($end > 0) {
                    if ($start_bb != bytekit_opline_to_bb($func, $end)) break;
                    $end--;
                }
                
                $is_safe = bytekit_trace_backward($func, $start-1, $end, $o[$i]['op1']);
                
                if ($is_safe == BYTEKIT_IS_UNSAFE) {
                    $found[] = $o[$i]['lineno'];
                }
            }
        }
    }

    if (count($found) == 0) die();

    sort($found);

    $fileData = file($fname);

    for ($i=0; $i<count($found); $i++) {
        echo $fname,"(",$found[$i],"): ",trim($fileData[$found[$i]-1]),"\n";
    }

    function bytekit_opline_to_bb(&$func, $opline)
    {
        $c = &$func['code'];
        
        for ($i=0; $i<count($c); $i++) {
            if ($c[$i]['opline'] == $opline) {
                return $func['bb'][$i];
            }
        }
        
        return FALSE;
    }

    function bytekit_equal_node(&$n1, &$n2)
    {
        return (($n1['type'] == $n2['type']) && ($n1['var'] == $n2['var']));
    }

    function bytekit_trace_backward(&$func, $start, $end, &$s)
    {
        $o = &$func['raw']['opcodes'];
        
        if ($s['type'] == BYTEKIT_IS_CONST) {
            return BYTEKIT_IS_SAFE;
        }
        
        $i = $start;
        while ($i >= $end)
        {
            $ol = &$o[$i];
            $opcode = $ol['opcode'];

            switch ($opcode) {
                case BYTEKIT_INCLUDE_OR_EVAL:
                    if (bytekit_equal_node($s, $ol['result'])) {
                        return BYTEKIT_IS_UNSAFE;
                    }
                    break;
                case BYTEKIT_FETCH_CONSTANT:
                    if (bytekit_equal_node($s, $ol['result'])) {
                        return BYTEKIT_IS_SAFE;
                    }
                    break;
                case BYTEKIT_ASSIGN:
                    if (bytekit_equal_node($s, $ol['op1'])) {
                        return bytekit_trace_backward($func, $i-1, $end, $ol['op2']);
                    }
                    break;
                case BYTEKIT_ASSIGN_CONCAT:
                    if (bytekit_equal_node($s, $ol['op1'])) {
                        $res1 = bytekit_trace_backward($func, $i-1, $end, $ol['op1']) == BYTEKIT_IS_SAFE;
                        $res2 = bytekit_trace_backward($func, $i-1, $end, $ol['op2']) == BYTEKIT_IS_SAFE;
                        return ($res1 && $res2) ? BYTEKIT_IS_SAFE : BYTEKIT_IS_UNSAFE;
                    }
                    break;
                case BYTEKIT_CONCAT:
                    if (bytekit_equal_node($s, $ol['result'])) {
                        $res1 = bytekit_trace_backward($func, $i-1, $end, $ol['op1']) == BYTEKIT_IS_SAFE;
                        $res2 = bytekit_trace_backward($func, $i-1, $end, $ol['op2']) == BYTEKIT_IS_SAFE;
                        return ($res1 && $res2) ? BYTEKIT_IS_SAFE : BYTEKIT_IS_UNSAFE;
                    }
                    break;
                default: return BYTEKIT_IS_UNSAFE;
            }
            $i--;
        }
        
        return BYTEKIT_IS_UNSAFE;
    }
?>
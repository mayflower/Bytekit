<?php

    ini_set("memory_limit", -1);

    if (extension_loaded("Bytekit") === false) {
        dl('bytekit.so');
    }

    $fname = $_SERVER['argv'][1];

    $arr = bytekit_disassemble_file($fname);
    $outname = $_SERVER['argv'][2];

    mkdir($outname);

    $cnt = 1;

    $f_i = 0;
    $f_cnt = count($arr);
    foreach($arr['functions'] as $functionname => $function) {
        $f_i++;
    
        $dc = eliminateDeadCode($function);
    
        $nodes = array();
    
        $filefname = str_replace("/", "_", $functionname);

        $fout = fopen($outname."/".$filefname.".dot", "w+");
        if (!$fout) die();
        fprintf($fout, "digraph flowgraph {
node [
    fontname=\"Courier\"
    fontsize=\"12\"
    shape=\"plaintext\"
];
graph [
    rankdir=\"HR\"
    bgcolor=\"#f7f7f7\"
    label=\"Control Flow Graph for $functionname()\"
    labeljust=\"c\"
    labelloc=\"t\"
    fontname=\"Courier\"
    fontsize=\"16\"
];
mindist = 0.4;
overlap = false;
");    
        
    $code = &$function['code'];
    $bb   = &$function['bb'];
    $bbid = -1;
    for ($i=0; $i<count($code); $i++) {
        $instruction = &$code[$i];
        
        /* check for new basic block */
        if ($bbid != $bb[$i]) {
            $bbid = $bb[$i];
            if (in_array($bbid, $dc)) continue;
            $nodes[$bbid] = array();
            $nodes[$bbid]['address'] = $instruction['address'];
            $nodes[$bbid]['instructions'] = array();
            $nodes[$bbid]['id'] = $cnt++;
        }
        if (in_array($bbid, $dc)) continue;
        $instr = array();
        $instr['address'] = $instruction['address'];
        $instr['mnemonic'] = $instruction['mnemonic'];
        $instr['operands'] = '';
        for ($j = 0; $j<count($instruction['operands']); $j++) {
            $instr['operands'] .= ($j != 0) ? ', ' : '';
            $instr['operands'] .= $instruction['operands'][$j]['string'];
        }
        $nodes[$bbid]['instructions'][] = $instr;
                
    }
    
    foreach ($nodes as $key => $value) {
        $bgcolor = "#FF8888";
        
        fprintf($fout, '"bb_%d" [', $key);
        fprintf($fout, "\n  label =<<TABLE BORDER=\"2\" CELLBORDER=\"0\" CELLSPACING=\"0\" BGCOLOR=\"#ffffff\">\n");
        fprintf($fout, "<TR><TD BGCOLOR=\"$bgcolor\" colspan=\"3\" ALIGN=\"LEFT\"><FONT face=\"Courier-Bold\" ");
        fprintf($fout, "point-size=\"12\">%s:</FONT></TD></TR>\n", sprintf("%08x",$nodes[$key]['address']));
        foreach($nodes[$key]['instructions'] as $instr) {
            fprintf($fout, "<TR><TD ALIGN=\"LEFT\">%s</TD><TD ALIGN=\"LEFT\">%s</TD><TD ALIGN=\"LEFT\">%s</TD></TR>\n", 
                           htmlentities(sprintf("%08x",$instr['address'])), 
                           htmlentities($instr['mnemonic']), 
                           htmlentities($instr['operands']));
        }
        fprintf($fout, "</TABLE>>\n");
        fprintf($fout, "];\n");
    }

    $cfg = &$function['cfg'];

    $doms = createDominatorTree($cfg);
    
    $loopEdge = array();
    $dfsOrder = array();
    $dfsId = 1;
    $depths = array();
    detectLoops($cfg, 1, $dummy=array(), 1);
    
    for ($i=1; $i<=count($cfg); $i++) {
        if (count($cfg[$i])==0) continue;
        foreach ($cfg[$i] as $key => $value) {
            if ($nodes[$i] == false) continue;
            
            switch ($value) {
                case BYTEKIT_EDGE_TRUE:      $style = 'color="#00ff00"'; break;
                case BYTEKIT_EDGE_FALSE:     $style = 'color="#ff0000"'; break;
                case BYTEKIT_EDGE_NORMAL:    $style = 'color="#000000"'; break;
                case BYTEKIT_EDGE_EXCEPTION: $style = 'style=dotted, penwidth=3.0, color="#0000ff"';break;
                default: $style = 'color="#0000ff"';
            }
            if (isBackEdge($cfg, $doms, $i, $key)) {
                $style = 'style=dashed, penwidth=3.0, ' . $style;
            }
            fprintf($fout, '"bb_%d" -> "bb_%d" [%s];'."\n", $i, $key, $style);
        }
    }

    fprintf($fout, "}\n");
    fclose($fout);
    system("dot -Tsvg -o $outname/$filefname.svg $outname/$filefname.dot");  
}

function eliminateDeadCode(&$f)
{
    $cfg = &$f['cfg'];
   // var_dump($cfg);
    $dcfound = false;
    $dc = array();
    do {
        $in = array();
        for ($i=1; $i<=count($cfg); $i++) {
            $in[$i] = 0;
        }
        for ($i=1; $i<=count($cfg); $i++) {
            foreach ($cfg[$i] as $child => $value) {
                $in[$child]++;
            }
        }
        $dcfound = false;
        for ($i=2; $i<=count($cfg); $i++) {
            if ($in[$i] == 0 && !in_array($i, $dc)) {
                $cfg[$i] = array();
                $dcfound = true;
                $dc[] = $i;
            }
        }
    } while ($dcfound == true);
    //var_dump($dc);
    return $dc;
}

function isBackEdge(&$cfg, &$doms, $n1, $n2)
{
    $postorderMap = createPostorderMapping($cfg);
    
    if ($n2 == 1) return true;
    if ($n1 == $n2) return true;
    $n1 = $postorderMap[$n1];
    $n2 = $postorderMap[$n2];
    
    while ($doms[$n1] != $n1) {
        if ($doms[$n1] == $n2) return true;
        $n1 = $doms[$n1];
    }
    return false;
}

function detectLoops(&$cfg, $node, &$visited, $depth)
{
    global $dfsOrder, $dfsId, $visitedBy, $depths;
    
    $visited[$node] = 1;
    $dfsOrder[$node] = $dfsId++;
    $depths[$node] = $depth;
    
    $childs = $cfg[$node];
    if (count($childs)==0) return;
    foreach ($childs as $cnode => $dummy) {
        if (isset($visited[$cnode])) {
            if ($depths[$cnode] < $depths[$node]) {
                $GLOBALS['loopEdge'][$node][$cnode] = 1;
            }
        } else {
            $visitedBy[$cnode] = $dfsOrder[$node];
            detectLoops($cfg, $cnode, $visited, $depth+1);
        }
    }
}

function createPredecessorArray(&$cfg)
{
    $predecessors = array();
    foreach ($cfg as $key => $value) {
        $predecessors[$key] = array();
    }
    /* build up predecessors */
    foreach ($cfg as $key => $node) {
        foreach ($node as $cnode => $dummy) {
            $predecessors[$cnode][] = $key;
        }
    }    
    return $predecessors;
}

function do_postorder(&$cfg, &$visited, &$postorder, $node)
{
    $succ = $cfg[$node];
    $visited[$node] = true;
    
    foreach ($succ as $cnode => $dummy) {
        if (!isset($visited[$cnode])) {
            do_postorder($cfg, $visited, $postorder, $cnode);
        }
    }
    $postorder[] = $node;
}

function createReversePostorder(&$cfg)
{
    $postorder = array();
    $visited   = array();
    
    do_postorder($cfg, $visited, $postorder, 1);
    
    return array_reverse($postorder);
}

function createPostorderMapping(&$cfg)
{
    $postorder = array();
    $visited   = array();
    $mapping   = array();
    
    do_postorder($cfg, $visited, $postorder, 1);
    
    for ($i=0; $i<count($postorder); $i++) {
        $mapping[$postorder[$i]] = $i+1;
    }
    
    return $mapping;
}

function createDominatorTree(&$cfg)
{
    $postorderMap = createPostorderMapping($cfg);

    /* create an empty dominator tree */
    $doms = array();
    foreach ($cfg as $key => $value) {
        if (isset($postorderMap[$key])) $doms[$postorderMap[$key]] = null;
    }
    
    /* Initialize algorithm */
    $start_node = $postorderMap[1];
    $doms[$start_node] = $start_node;
    $changed = true;
    $predecessors = createPredecessorArray($cfg);
    $reversePostorder = createReversePostorder($cfg);

    while ($changed) {
        $changed = false;
        foreach ($reversePostorder as $b) {
            if ($b == 1) continue;
            //echo "now: $b\n";
            $new_idom = null;
            //var_dump($predecessors[$b]);
            for ($i=0; $i<count($predecessors[$b]); $i++) {
                $p = $predecessors[$b][$i];
                $p = $postorderMap[$p];
                
                if ($doms[$p] !== null) {
                    $new_idom = $p;
                    //echo "new_idom is $new_idom\n";
                    break;
                }
            }
            if ($new_idom === null) {
                die("new_idom is null");
            }
            for ($i=0; $i<count($predecessors[$b]); $i++) {
                $p = $predecessors[$b][$i];
                $p = $postorderMap[$p];
                
                //echo "work on $p\n";
                if ($p == $new_idom) continue;
                if ($doms[$p] !== null) {
                    $new_idom = intersect($doms, $p, $new_idom);
                }
            }
            
            if ($doms[$postorderMap[$b]] != $new_idom) {
                $doms[$postorderMap[$b]] = $new_idom;
                $changed = true;
            }
        }
    }
    return $doms;
}

function intersect(&$doms, $b1, $b2)
{
    $finger1 = $b1;
    $finger2 = $b2;
    //var_dump($doms);
    //echo "b1: $b1 - b2: $b2\n";
    while ($finger1 != $finger2) {
        while ($finger1 < $finger2) {
            //echo "finger1: $finger1\n";
            $finger1 = $doms[$finger1];
        }
        while ($finger2 < $finger1) {
            //echo "finger2: $finger2\n";
            $finger2 = $doms[$finger2];
        }
        //echo "...\n";
    }
    return $finger1;
}
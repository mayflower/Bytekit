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
            $nodes[$bbid] = array();
            $nodes[$bbid]['address'] = $instruction['address'];
            $nodes[$bbid]['instructions'] = array();
            $nodes[$bbid]['id'] = $cnt++;
        }
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
            fprintf($fout, '"bb_%d" -> "bb_%d" [%s];'."\n", $i, $key, $style);
        }
    }

    fprintf($fout, "}\n");
    fclose($fout);
    system("dot -Tsvg -o $outname/$filefname.svg $outname/$filefname.dot");  
}
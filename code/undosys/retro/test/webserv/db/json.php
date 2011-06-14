<?php

/************************************************************
 * functions to serialize/deserialize php objects using JSON
 ************************************************************/

// encode resources as a special string
// _R_:res_type:res_id:_R_ with the assumption
// that no actual strings will be in this format
function encode_resource($var, &$resource_map=null) {
    $r = "_R_:" . get_resource_type($var) . ":" . ((int)$var);
    if ($resource_map !== null) {
        $resource_map[$r] = $var;
    }
    return $r;
}

function decode_resource($var, &$resource_map=null) {
    if (is_string($var) && preg_match('/^_R_:[^:]+:\d+$/', $var) == 1 &&
        $resource_map !== null) {
        return $resource_map[$var];
    } else {
        return $var;
    }
}

function json_serialize($var, &$resource_map=null) {
    if (is_resource($var)) {
        $var = encode_resource($var, $resource_map);
    }
       
    if (is_array($var)) {
        for ($i=0; $i < count($var); $i++) {
            if (is_resource($var[$i])) {
                $var[$i] = encode_resource($var[$i], $resource_map);
            }
        }
    }

    return rawurlencode(json_encode($var));
}

function json_deserialize($str, &$resource_map=null) {
    $var = json_decode(rawurldecode($str));

    if (is_array($var)) {
        for ($i=0; $i < count($var); $i++) {
            $var[$i] = decode_resource($var[$i], $resource_map);
        }
    } else {
        $var = decode_resource($var, $resource_map);
    }

    return $var;
}

?>
<?php
    require_once("zk_client.php");
    $cluster_name = "ci";
    $path = "/zk/ci/xcube/zk_client_node";
    
    $zk_client = new ZookeeperClient();
    $node = $zk_client->Create($path);
    $node->SetContent("tets_value");
    $json_value = new string_vector;
    $node->GetContentAsJson($json_value);
    $node->Delete();

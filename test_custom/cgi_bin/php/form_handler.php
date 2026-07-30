<?php
$method = $_SERVER['REQUEST_METHOD'];
$data = [];

if ($method === 'POST') {
    $data = array_merge($_POST, $_GET);
} else {
    $data = $_GET;
}

echo "<html><head><title>PHP Form Handler</title>";
echo "<style>body{font-family:monospace;margin:20px}table{border-collapse:collapse}th,td{border:1px solid #ddd;padding:8px}th{background:#9C27B0;color:white}</style>";
echo "</head><body>";
echo "<h1>PHP Form Data (Method: " . htmlspecialchars($method) . ")</h1>";
echo "<table><tr><th>Field</th><th>Value</th></tr>";
if (!empty($data)) {
    foreach ($data as $key => $value) {
        echo "<tr><td>" . htmlspecialchars($key) . "</td><td>" . htmlspecialchars($value) . "</td></tr>";
    }
} else {
    echo "<tr><td colspan='2'>No data</td></tr>";
}
echo "</table>";
echo "<p><a href='/php/form_handler.php'>Back</a></p>";
echo "</body></html>";
?>

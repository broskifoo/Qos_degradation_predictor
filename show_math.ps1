Write-Host "================================================================================" -ForegroundColor Cyan
Write-Host "                LOGISTIC REGRESSION MATHEMATICAL CALCULATIONS                   " -ForegroundColor Cyan
Write-Host "================================================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Logistic Regression predicts the probability of QoS Degradation (Label = 1) using a Sigmoid function:" -ForegroundColor White
Write-Host ""
Write-Host "   P(Y=1) = 1 / (1 + e^-z)" -ForegroundColor Yellow
Write-Host ""
Write-Host "where 'z' is the linear combination of weighted features:" -ForegroundColor White
Write-Host "   z = b + (w1 * x1) + (w2 * x2) + ... + (wn * xn)" -ForegroundColor Yellow
Write-Host ""

$weightsFile = Join-Path $PSScriptRoot "results\model_weights.txt"
if (Test-Path $weightsFile) {
    Write-Host "Reading trained model weights..." -ForegroundColor Green
    $lines = Get-Content $weightsFile
    $model = @{}
    foreach ($line in $lines) {
        if ($line -like "*,*") {
            $parts = $line.Split(",")
            $model[$parts[0]] = [double]$parts[1]
        }
    }

    $b = "{0:N4}" -f $model["Bias"]
    $w1 = "{0:N4}" -f $model["Weight_AvgRTT"]
    $w2 = "{0:N4}" -f $model["Weight_RTTVar"]
    $w3 = "{0:N4}" -f $model["Weight_Queue"]
    $w4 = "{0:N4}" -f $model["Weight_Loss"]
    $w5 = "{0:N4}" -f $model["Weight_Tput"]
    $w6 = "{0:N4}" -f $model["Weight_Jitter"]

    Write-Host ""
    Write-Host "Trained weights for the model:" -ForegroundColor Cyan
    Write-Host "--------------------------------"
    Write-Host "Bias (b)         = $b"
    Write-Host "Avg RTT (w1)     = $w1"
    Write-Host "RTT Var (w2)     = $w2"
    Write-Host "Queue Occ (w3)   = $w3"
    Write-Host "Pkt Loss (w4)    = $w4"
    Write-Host "Throughput (w5)  = $w5"
    Write-Host "Jitter (w6)      = $w6"
    Write-Host "--------------------------------"

    Write-Host ""
    Write-Host "Formula for 'z':" -ForegroundColor Yellow
    Write-Host "z = $b + ($w1 * AvgRTT) + ($w2 * RTTVar) + ($w3 * QueueOcc) + ($w4 * PktLoss) + ($w5 * Tput) + ($w6 * Jitter)" -ForegroundColor Yellow

    Write-Host ""
    Write-Host "Example Calculation for a severely congested network state:" -ForegroundColor White
    $ex_rtt = 150.0  # ms
    $ex_var = 50.0   # ms^2
    $ex_queue = 90.0 # packets
    $ex_loss = 0.05  # 5%
    $ex_tput = 50000000.0 # 50 Mbps
    $ex_jitter = 30.0 # ms

    Write-Host "Features -> AvgRTT: $ex_rtt, RTTVar: $ex_var, QueueOcc: $ex_queue, PktLoss: $ex_loss, Tput: $($ex_tput/1MB) Mbps, Jitter: $ex_jitter"
    
    $z = $model["Bias"] + 
         ($model["Weight_AvgRTT"] * $ex_rtt) + 
         ($model["Weight_RTTVar"] * $ex_var) + 
         ($model["Weight_Queue"] * $ex_queue) + 
         ($model["Weight_Loss"] * $ex_loss) + 
         ($model["Weight_Tput"] * $ex_tput) + 
         ($model["Weight_Jitter"] * $ex_jitter)
         
    $z_str = "{0:N4}" -f $z
    $prob = 1.0 / (1.0 + [math]::Exp(-$z))
    $prob_str = "{0:N2}%" -f ($prob * 100.0)

    Write-Host "Calculation:"
    Write-Host "z = $z_str" -ForegroundColor Magenta
    Write-Host "P(Degradation) = 1 / (1 + e^-($z_str)) = $prob_str" -ForegroundColor Green
    
    if ($prob -ge 0.5) {
        Write-Host "-> Prediction: DEGRADED QOS" -ForegroundColor Red
    } else {
        Write-Host "-> Prediction: OPTIMAL QOS" -ForegroundColor Green
    }
} else {
    Write-Host "Error: Model weights not found. Please run the simulation and train the model first." -ForegroundColor Red
}

Write-Host ""
Write-Host "================================================================================" -ForegroundColor Cyan
Write-Host "Press any key to exit..."
$Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown") | Out-Null

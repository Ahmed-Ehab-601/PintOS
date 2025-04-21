## 🧠 Priority Scheduling Formulas

### 🔢 Priority Range
- **Range**: `[0, 63]`  
- **Calculation Formula**:  
`priority = PRI_MAX - (recent_cpu / 4) - (nice * 2)`
- **Calculated At**:
- During **thread initialization**
- Every **4th clock tick** for **every thread**

---

### 😊 Nice Value

- **Range**: `[-20, 20]`  
- **Default**: `0` or **inherited** from the parent
- **Definition**:  
A value that determines how *"nice"* a thread is to other threads — higher values mean the thread yields more CPU time to others.

---

### ⏱️ recent_cpu

- **Type**: `int` (Can be negative)  
- **Default**: `0` or **inherited**
- **Definition**:  
A measure of how much CPU time a thread has received recently.

#### 📈 Formula:
`recent_cpu = (2 * load_avg) / (2 * load_avg + 1) * recent_cpu + nice`

#### 🕒 Updated:
- Every **timer interrupt**, incremented as:
running_thread->recent_cpu++;
*(only if `running_thread != idle_thread`)*
- Recalculated **once per second** for **running**, **ready**, or **blocked** threads using the above formula

---

### 📊 load_avg

- **Scope**: **Global** (Not thread-specific)  
- **Default**: `0`
- **Definition**:  
An estimate of the average number of threads ready to run over the past minute.

#### 📉 Formula:
`load_avg = (59/60) * load_avg + (1/60) * ready_threads`

#### 🕐 Updated:
- Calculated **once per second** (every **100 ticks**)

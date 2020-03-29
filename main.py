from __future__ import annotations

from typing import List, Tuple, Dict, DefaultDict
from collections import defaultdict
from enum import Enum
from pprint import pprint
from itertools import product

class Gate:
    inputs: List[GateInput]
    outputs: List[GateOutput]
    delay: int

    def __init__(self):
        self.inputs = []
        self.outputs = []
        self.delay = 1

    def evaluate(self, inputs: Tuple[bool]) -> Tuple[bool]:
        raise NotImplementedError()

class GateSocket:
    def __init__(self, gate: Gate, index: int):
        self.gate = gate
        self.index = index

class GateInput(GateSocket):
    def __init__(self, gate: Gate, index: int):
        super().__init__(gate, index)
        self.origin = None

class GateOutput(GateSocket):
    def __init__(self, gate: Gate, index: int):
        super().__init__(gate, index)
        self.targets = []

class NotGate(Gate):
    def __init__(self):
        super().__init__()
        self.inputs.append(GateInput(self, 0))
        self.outputs.append(GateOutput(self, 0))

    def evaluate(self, inputs):
        return (not inputs[0], )

class AndGate(Gate):
    def __init__(self, n=2):
        super().__init__()
        for i in range(n):
            self.inputs.append(GateInput(self, i))
        self.outputs.append(GateOutput(self, 0))

    def evaluate(self, inputs):
        return (all(inputs), )

class ConstGate(Gate):
    def __init__(self, value: bool):
        super().__init__()
        self.value = value
        self.outputs.append(GateOutput(self, 0))

    def evaluate(self, inputs):
        return (self.value, )

def make_link(gate_output: GateOutput, gate_input: GateInput):
    assert gate_input not in gate_output.targets
    assert gate_input.origin is None
    gate_output.targets.append(gate_input)
    gate_input.origin = gate_output

def make_input_links(gate: Gate, *gate_outputs: GateOutput):
    assert len(gate.inputs) == len(gate_outputs)
    for gate_input, gate_output in zip(gate.inputs, gate_outputs):
        make_link(gate_output, gate_input)

const1 = ConstGate(False).outputs[0]
const2 = ConstGate(False).outputs[0]

and_gate = AndGate(2)
not_gate = NotGate()

make_input_links(and_gate, const1, const2)
make_input_links(not_gate, and_gate.outputs[0])

def find_all_gates(any_gate: Gate):
    gates = set()

    def find(gate: Gate):
        if gate in gates:
            return
        gates.add(gate)
        for socket in gate.inputs:
            if socket.origin is not None:
                find(socket.origin.gate)
        for socket in gate.outputs:
            for target in socket.targets:
                find(target.gate)

    find(any_gate)

    return gates

class Event:
    def __init__(self, gate, input_states):
        self.gate = gate
        self.input_states = input_states

def simulate_tick(current_tick: int, all_output_states: Dict[GateOutput, bool], events_by_tick: DefaultDict[int, List[Event]]):
    current_events = events_by_tick.pop(current_tick, [])
    gates_to_trigger = set()

    for event in current_events:
        new_output_states = event.gate.evaluate(event.input_states)
        for gate_output, new_state in zip(event.gate.outputs, new_output_states):
            all_output_states[gate_output] = new_state
            for target in gate_output.targets:
                gates_to_trigger.add(target.gate)

    for gate in gates_to_trigger:
        input_states = tuple(all_output_states[input.origin] for input in gate.inputs)
        new_event = Event(gate, input_states)
        events_by_tick[current_tick + gate.delay].append(new_event)

def simulate_until_done(all_output_states: Dict[GateOutput, bool], events_by_tick: DefaultDict[int, List[Event]]):
    tick = 0
    while len(events_by_tick) > 0:
        simulate_tick(tick, output_states, events)
        tick += 1
    return tick


for in1, in2 in product([False, True], repeat=2):
    events = defaultdict(list)
    output_states = dict()
    gates = find_all_gates(and_gate)

    for gate in gates:
        for socket in gate.outputs:
            output_states[socket] = False

    const1.gate.value = in1
    const2.gate.value = in2
    events[0].append(Event(const1.gate, ()))
    events[0].append(Event(const2.gate, ()))
    ticks = simulate_until_done(output_states, events)
    print("Required Ticks:", ticks)
    pprint(output_states)

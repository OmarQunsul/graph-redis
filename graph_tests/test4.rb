# Deleting edges

require 'redis'

redis = Redis.new

describe 'Removing edges' do

  it 'should correctly remove edges from undirected graphs' do
    redis.flushdb
    redis.gvertex 'graph1', 'a', 'b', 'c', 'd'
    redis.gedge 'graph1', 'a', 'b', 1
    redis.gedge 'graph1', 'b', 'c', 1
    redis.gedge 'graph1', 'c', 'd', 1
    redis.gedge 'graph1', 'a', 'd', 2
    redis.gedgerem 'graph1', 'a', 'd'
    path = redis.gshortestpath 'graph1', 'a', 'd'
    path.last.should eq '3'
    path = redis.gshortestpath 'graph1', 'd', 'a'
    path.last.should eq '3'
    edges = redis.gedges 'graph1'
    edges.length.should eq (3 * 3)
  end

  it 'should correctly remove edges from directed graphs' do
    redis.flushdb
    redis.gvertex 'graph1', 'a', 'b', 'c', 'd'
    redis.gsetdirected 'graph1'
    redis.gedge 'graph1', 'a', 'b', 1
    redis.gedge 'graph1', 'c', 'b', 1
    redis.gedge 'graph1', 'c', 'd', 1
    redis.gedge 'graph1', 'd', 'b', 1
    redis.gedge 'graph1', 'b', 'c', 1
    incoming = redis.gincoming 'graph1', 'b'
    incoming.length.should eq 3
    redis.gedgerem('graph1', 'a', 'c').should eq 0
    redis.gedgerem('graph1', 'c', 'b').should eq 1
    incoming = redis.gincoming 'graph1', 'b'
    incoming.length.should eq 2

    path = redis.gshortestpath 'graph1', 'c', 'b'
    path.last.should eq '2'
  end

end


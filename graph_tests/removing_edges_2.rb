# edges exists

require 'redis'

redis = Redis.new

describe 'Removing edges' do

  it 'should correctly remove edges from undirected graphs' do
    redis.flushdb
    redis.gvertex 'graph1', 'a', 'b', 'c', 'd'
    redis.gedge 'graph1', 'a', 'b', 1
    redis.gedge 'graph1', 'b', 'c', 1
    redis.gedgeexists('graph1', 'a', 'b').should eq 1
    redis.gedgeexists('graph1', 'b', 'a').should eq 1
    redis.gedgerem('graph1', 'b', 'a').should eq 1
    redis.gedgeexists('graph1', 'a', 'b').should eq 0
    redis.gedgeexists('graph1', 'b', 'a').should eq 0
  end

end


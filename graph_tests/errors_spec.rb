require 'redis'

redis = Redis.new

describe 'wrong type' do
  before do
    redis.flushdb
  end

  it "should return error while adding node to non-graph key" do
    redis.set("graph1", "value1")
    ret = redis.type('graph1')
    expect(ret).to eq('string')
    expect do
      redis.gnode("graph1", "v1")
    end.to raise_exception(Redis::CommandError, /WRONGTYPE/)
  end

  it "should return error while adding an edge to null key" do
    expect do
      redis.gedge("graph1", "v1", "v2", 10)
    end.to raise_exception(Redis::CommandError, /WRONGTYPE/)
  end

  it "should return error while adding an edge to non-graph key" do
    redis.set("graph1", "value1")
    ret = redis.type('graph1')
    expect(ret).to eq('string')
    expect do
      redis.gedge("graph1", "v1", "v2", 10)
    end.to raise_exception(Redis::CommandError, /WRONGTYPE/)
  end

  it "should return of graph shortest distance asks for nodes that don't exist" do
    redis.gnode "graph1", "a"
    expect do
      path = redis.gshortestpath "graph1", "a", "b"
    end.to raise_exception(Redis::CommandError, /Node not found/)
  end
end

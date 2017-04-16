require 'redis'

redis = Redis.new

describe 'basic commands' do
  before do
    redis.flushdb
    redis.gnode 'graph1', 'a', 'b', 'c', 'd'
    redis.gsetdirected 'graph1'
    redis.gedge 'graph1', 'a', 'b', 2
    redis.gedge 'graph1', 'a', 'c', 1
    redis.gedge 'graph1', 'd', 'b', 1
  end

  it "should calculate the outdegree correctly" do
    a = redis.gnodeoutdegree 'graph1', 'a'
    expect(a).to eq(2)
    b = redis.gnodeoutdegree 'graph1', 'b'
    expect(b).to eq(0)
    d = redis.gnodeoutdegree 'graph1', 'd'
    expect(d).to eq(1)
  end

  it "should calculate the indegree correctly" do
    a = redis.gnodeindegree 'graph1', 'a'
    expect(a).to eq(0)
    b = redis.gnodeindegree 'graph1', 'b'
    expect(b).to eq(2)
    d = redis.gnodeindegree 'graph1', 'd'
    expect(d).to eq(0)
  end
end
